from pathlib import Path
import shutil

ROOT = Path('.')
OLD = ROOT / 'components' / 'toshiba_suzumi'
NEW = ROOT / 'components' / 'toshiba_a2a'

if NEW.exists():
    shutil.rmtree(NEW)
shutil.move(str(OLD), str(NEW))

# Rename ESPHome component namespace/path references in code.
for p in NEW.rglob('*'):
    if p.is_file() and p.suffix in {'.h', '.cpp', '.py'}:
        s = p.read_text()
        s = s.replace('toshiba_suzumi', 'toshiba_a2a')
        p.write_text(s)

# Consumer-facing component references. Historical provenance text is deliberately untouched.
for rel in [
    'packages/toshiba-a2a-j2.yaml',
    'packages/toshiba-a2a-p2.yaml',
    'examples/output_estimation.yaml',
    'examples/diagnostic_capture.yaml',
    'examples/engineering_telemetry.yaml',
    'components/toshiba_output/__init__.py',
    'components/toshiba_output/toshiba_output.h',
]:
    p = ROOT / rel
    s = p.read_text().replace('toshiba_suzumi', 'toshiba_a2a')
    p.write_text(s)

# Split device/model profile data from E0 identity decoding.
old_h = NEW / 'toshiba_model.h'
old_cpp = NEW / 'toshiba_model.cpp'
h = old_h.read_text()
cpp = old_cpp.read_text()

struct_start = h.index('struct ToshibaEquipmentIdentification {')
struct_end = h.index('};', struct_start) + 3
# Remove the E0 decoder declaration and its large explanatory comment with the structure.
dec = 'ToshibaEquipmentIdentification decode_equipment_identification(const std::vector<uint8_t> &raw_data);'
dec_pos = h.index(dec)
comment_start = h.rfind('/**', struct_end, dec_pos)
if comment_start < 0:
    comment_start = struct_start
h = h[:struct_start] + h[struct_end:comment_start] + h[dec_pos + len(dec):]
h = h.replace('#include <vector>\n', '')
h = h.replace('toshiba_model.h', 'toshiba_device_profile.h')

# Add the only currently proven family-dependent wire encoding as declarative profile data.
insert_after = 'enum class ToshibaHvacMode : uint8_t {\n  UNKNOWN = 0,\n  AUTO,\n  COOL,\n  HEAT,\n  DRY,\n  FAN,\n};\n'
louvre_enum = '''\n// A3 is the only currently proven family-dependent command encoding.\nenum class ToshibaLouvreEncoding : uint8_t {\n  UNKNOWN = 0,\n  J2_VERTICAL,\n  P2_PACKED,\n};\n'''
h = h.replace(insert_after, insert_after + louvre_enum)

# Promote profile queries into the profile interface so control code asks about capabilities/encoding,
# rather than embedding model/family policy.
h = h.replace('ToshibaCapabilityProfile capability_profile_from_model(const std::string &model);\n',
              'ToshibaCapabilityProfile capability_profile_from_model(const std::string &model);\n'
              'ToshibaLouvreEncoding louvre_encoding_for_family(ToshibaIndoorUnitFamily family);\n'
              'bool has_validated_mode_profile(ToshibaIndoorUnitFamily family);\n'
              'bool validated_strong_defrost_allowed(ToshibaIndoorUnitFamily family, ToshibaHvacMode mode);\n')

profile_h = NEW / 'toshiba_device_profile.h'
profile_h.write_text(h)

# Extract helpers needed only by E0 identity decoding.
helper_start = cpp.index('std::string decode_ascii_field')
profile_start = cpp.index('constexpr uint32_t COMMON_RESIDENTIAL_FEATURES')
helper_block = cpp[helper_start:profile_start]
# Anonymous namespace started before helpers; profile data must retain its own anonymous namespace.
decode_start = cpp.index('ToshibaEquipmentIdentification decode_equipment_identification')
namespace_close = cpp.rfind('\n}  // namespace toshiba_a2a\n}  // namespace esphome')
decode_block = cpp[decode_start:namespace_close]

profile_body = cpp[profile_start:decode_start]
# Ensure the anonymous namespace that contained the static profile tables exists after extraction.
profile_cpp = '''#include "toshiba_device_profile.h"\n\nnamespace esphome {\nnamespace toshiba_a2a {\n\nnamespace {\n\n''' + profile_body
# profile_body already contains the anonymous namespace close before public functions.
profile_cpp += '''\n}  // namespace toshiba_a2a\n}  // namespace esphome\n'''

# Extend profile implementation with concise encoding/policy queries.
needle = '''ToshibaCapabilityProfile capability_profile_from_model(const std::string &model) {\n  ToshibaCapabilityProfile profile;\n  const auto family = indoor_unit_family_from_model(model);\n\n  for (const auto &entry : TOSHIBA_FAMILY_CAPABILITIES) {\n    if (entry.family == family) {\n      profile.features = entry.features;\n      return profile;\n    }\n  }\n\n  profile.features = FEATURE_COMMON_HVAC;\n  return profile;\n}\n'''
addition = needle + '''\nToshibaLouvreEncoding louvre_encoding_for_family(ToshibaIndoorUnitFamily family) {\n  switch (family) {\n    case ToshibaIndoorUnitFamily::J2FVG: return ToshibaLouvreEncoding::J2_VERTICAL;\n    case ToshibaIndoorUnitFamily::P2KVSG: return ToshibaLouvreEncoding::P2_PACKED;\n    default: return ToshibaLouvreEncoding::UNKNOWN;\n  }\n}\n\nbool has_validated_mode_profile(ToshibaIndoorUnitFamily family) {\n  for (const auto &entry : TOSHIBA_VALIDATED_MODE_FUNCTIONS) {\n    if (entry.family == family) return true;\n  }\n  return false;\n}\n\nbool validated_strong_defrost_allowed(ToshibaIndoorUnitFamily family, ToshibaHvacMode mode) {\n  return family == ToshibaIndoorUnitFamily::P2KVSG && mode == ToshibaHvacMode::HEAT;\n}\n'''
if needle not in profile_cpp:
    raise RuntimeError('capability_profile_from_model block not found')
profile_cpp = profile_cpp.replace(needle, addition)
(NEW / 'toshiba_device_profile.cpp').write_text(profile_cpp)

identity_h = '''#pragma once\n\n#include <cstdint>\n#include <string>\n#include <vector>\n\nnamespace esphome {\nnamespace toshiba_a2a {\n\nstruct ToshibaEquipmentIdentification {\n  bool valid{false};\n  bool idu_model_available{false};\n  bool odu_model_available{false};\n  std::string idu_model;\n  std::string idu_identity_1;\n  std::string idu_identity_2;\n  std::string idu_identity_3;\n  std::string odu_model;\n  std::string odu_identity_1;\n  std::string odu_identity_2;\n  std::string odu_identity_3;\n};\n\n// Decode the pushed class-0x11 / register-0xE0 equipment-identification payload.\nToshibaEquipmentIdentification decode_equipment_identification(const std::vector<uint8_t> &raw_data);\n\n}  // namespace toshiba_a2a\n}  // namespace esphome\n'''
(NEW / 'toshiba_identity.h').write_text(identity_h)

identity_cpp = '''#include "toshiba_identity.h"\n\n#include <cctype>\n\nnamespace esphome {\nnamespace toshiba_a2a {\n\nnamespace {\n\n''' + helper_block + '''\n}  // namespace\n\n''' + decode_block + '''\n\n}  // namespace toshiba_a2a\n}  // namespace esphome\n'''
(NEW / 'toshiba_identity.cpp').write_text(identity_cpp)

old_h.unlink()
old_cpp.unlink()

# Update includes after the split.
for p in NEW.rglob('*'):
    if p.is_file() and p.suffix in {'.h', '.cpp'}:
        s = p.read_text().replace('"toshiba_model.h"', '"toshiba_device_profile.h"')
        if 'ToshibaEquipmentIdentification' in s or 'decode_equipment_identification' in s:
            if '#include "toshiba_identity.h"' not in s:
                # Keep local Toshiba includes together.
                marker = '#include "toshiba_device_profile.h"\n'
                if marker in s:
                    s = s.replace(marker, marker + '#include "toshiba_identity.h"\n', 1)
                else:
                    s = '#include "toshiba_identity.h"\n' + s
        p.write_text(s)

# Remove the duplicate local helper and replace direct family tests with profile/encoding queries.
vc = NEW / 'toshiba_validated_controls.cpp'
s = vc.read_text()
local_helper = '''bool has_validated_mode_profile(ToshibaIndoorUnitFamily family) {\n  return family == ToshibaIndoorUnitFamily::J2FVG || family == ToshibaIndoorUnitFamily::P2KVSG;\n}\n\n'''
s = s.replace(local_helper, '')

s = s.replace('if (this->idu_family_ == ToshibaIndoorUnitFamily::P2KVSG && mode != ToshibaHvacMode::HEAT) {',
              'if (!validated_strong_defrost_allowed(this->idu_family_, mode)) {')
s = s.replace('if (this->idu_family_ == ToshibaIndoorUnitFamily::J2FVG) {\n    const uint8_t raw = static_cast<uint8_t>(0x4F + requested_vertical);  // 50..54',
              'if (louvre_encoding_for_family(this->idu_family_) == ToshibaLouvreEncoding::J2_VERTICAL) {\n    const uint8_t raw = static_cast<uint8_t>(0x4F + requested_vertical);  // 50..54')
s = s.replace('if (this->idu_family_ != ToshibaIndoorUnitFamily::P2KVSG) {\n    ESP_LOGW(TAG, "Vertical FIX requested with unknown/unsupported IDU family");',
              'if (louvre_encoding_for_family(this->idu_family_) != ToshibaLouvreEncoding::P2_PACKED) {\n    ESP_LOGW(TAG, "Vertical FIX requested with unknown/unsupported louvre encoding");')
s = s.replace('if (this->idu_family_ != ToshibaIndoorUnitFamily::P2KVSG) {\n    ESP_LOGW(TAG, "Horizontal FIX is only implemented for the P2 family");',
              'if (louvre_encoding_for_family(this->idu_family_) != ToshibaLouvreEncoding::P2_PACKED) {\n    ESP_LOGW(TAG, "Horizontal FIX is unavailable for this louvre encoding");')
s = s.replace('if (this->idu_family_ == ToshibaIndoorUnitFamily::J2FVG && call.get_swing_mode().has_value()) {',
              'if (louvre_encoding_for_family(this->idu_family_) == ToshibaLouvreEncoding::J2_VERTICAL &&\n      call.get_swing_mode().has_value()) {')
s = s.replace('if (this->idu_family_ == ToshibaIndoorUnitFamily::J2FVG) {\n      if (value >= 0x50 && value <= 0x54) {',
              'if (louvre_encoding_for_family(this->idu_family_) == ToshibaLouvreEncoding::J2_VERTICAL) {\n      if (value >= 0x50 && value <= 0x54) {')
s = s.replace('if (this->idu_family_ == ToshibaIndoorUnitFamily::P2KVSG)\n      this->publish_horizontal_air_direction_(value);',
              'if (louvre_encoding_for_family(this->idu_family_) == ToshibaLouvreEncoding::P2_PACKED)\n      this->publish_horizontal_air_direction_(value);')
vc.write_text(s)

# Update comments/classes to refer to Toshiba A2A rather than the inherited Suzumi project name.
for p in NEW.rglob('*'):
    if p.is_file() and p.suffix in {'.h', '.cpp', '.py'}:
        s = p.read_text().replace('Suzumi', 'A2A').replace('SUZUMI', 'A2A')
        p.write_text(s)

# Branch-only compile workflow: test both canonical package templates after refactor.
wf = ROOT / '.github' / 'workflows' / 'refactor-toshiba-a2a.yml'
wf.write_text('''name: Refactor Toshiba A2A compile test\n\non:\n  push:\n    branches:\n      - refactor/toshiba-protocol-model-separation\n\njobs:\n  compile:\n    runs-on: ubuntu-latest\n    steps:\n      - uses: actions/checkout@v4\n      - uses: actions/setup-python@v5\n        with:\n          python-version: "3.12"\n      - name: Install ESPHome\n        run: python -m pip install --upgrade esphome\n      - name: Create test secrets\n        run: |\n          cat > examples/secrets.yaml <<'EOF'\n          wifi_ssid: test-wifi\n          wifi_password: test-password\n          iot_wifi_ssid: test-iot\n          iot_wifi_password: test-password\n          iot2_wifi_ssid: test-iot2\n          iot2_wifi_password: test-password\n          fallback_password: test-fallback\n          key: MDEyMzQ1Njc4OWFiY2RlZjAxMjM0NTY3ODlhYmNkZWY=\n          ota_password: test-ota\n          EOF\n      - name: Compile Kitchen P2\n        run: esphome compile examples/kitchen-package-template.yaml\n      - name: Compile Office J2\n        run: esphome compile examples/office-j2-package-template.yaml\n''')

# Changelog entry for this branch refactor.
ch = ROOT / 'CHANGELOG.txt'
text = ch.read_text()
marker = 'Sep 16th 2026\n\n'
entry = ('* branch refactor: renamed the ESPHome component/namespace from inherited `toshiba_suzumi` to `toshiba_a2a`, '
         'separated E0 identity decoding from declarative device-profile data, and routed proven A3 family differences through a louvre-encoding profile rather than direct family checks in control code\n')
if entry not in text:
    text = text.replace(marker, marker + entry, 1)
ch.write_text(text)
