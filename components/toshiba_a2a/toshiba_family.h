#pragma once
#include <cstddef>
#include <cstdint>
namespace esphome { namespace toshiba_a2a {
enum class ToshibaIndoorUnitFamily:uint8_t { UNKNOWN=0,J2FVG,P2KVSG };
enum class ToshibaHvacMode:uint8_t { UNKNOWN=0,AUTO,COOL,HEAT,DRY,FAN };
enum class ToshibaLouvreEncoding:uint8_t { UNKNOWN=0,J2_VERTICAL,P2_PACKED };
enum ToshibaFeature:uint32_t {
 FEATURE_NONE=0,FEATURE_COMMON_HVAC=1UL<<0,FEATURE_ECO=1UL<<1,FEATURE_HI_POWER=1UL<<2,
 FEATURE_COMFORT_SLEEP=1UL<<3,FEATURE_POWER_SELECT=1UL<<4,FEATURE_OUTDOOR_SILENT=1UL<<5,
 FEATURE_FIREPLACE=1UL<<6,FEATURE_EIGHT_DEG_HEAT=1UL<<7,FEATURE_VERTICAL_AIRFLOW=1UL<<8,
 FEATURE_HORIZONTAL_AIRFLOW=1UL<<9,FEATURE_FLOOR=1UL<<10,FEATURE_AIR_OUTLET_SELECT=1UL<<11,
 FEATURE_HADA_CARE=1UL<<12,FEATURE_SLEEP=1UL<<13,FEATURE_COMFORT=1UL<<14,
 FEATURE_PURE=1UL<<15,FEATURE_START_DEFROST=1UL<<16
};
enum class ToshibaFeatureScope:uint8_t { UNKNOWN=0,IDU_LOCAL,IDU_DEMAND,SHARED_ODU };
enum ToshibaFanOption:uint8_t { FAN_OPTION_NONE=0,FAN_OPTION_MANUAL=1U<<0,FAN_OPTION_AUTO=1U<<1,FAN_OPTION_QUIET=1U<<2 };
struct ToshibaCapabilityProfile { uint32_t features{FEATURE_NONE}; constexpr bool has(ToshibaFeature f) const { return (features & static_cast<uint32_t>(f))!=0; } };
struct ToshibaModeProfile { ToshibaHvacMode mode; ToshibaCapabilityProfile functions; uint8_t fan_options; };
struct ToshibaFamilyProfile { ToshibaIndoorUnitFamily family; ToshibaLouvreEncoding louvre_encoding; ToshibaCapabilityProfile capabilities; const ToshibaModeProfile *mode_profiles; size_t mode_profile_count; };
inline constexpr ToshibaFamilyProfile UNKNOWN_FAMILY_PROFILE{ToshibaIndoorUnitFamily::UNKNOWN,ToshibaLouvreEncoding::UNKNOWN,ToshibaCapabilityProfile{FEATURE_COMMON_HVAC},nullptr,0};
inline const ToshibaModeProfile *mode_profile_for(const ToshibaFamilyProfile &p,ToshibaHvacMode m){for(size_t i=0;i<p.mode_profile_count;i++)if(p.mode_profiles[i].mode==m)return &p.mode_profiles[i];return nullptr;}
inline bool has_validated_mode_profile(const ToshibaFamilyProfile &p){return p.mode_profiles!=nullptr&&p.mode_profile_count!=0;}
inline ToshibaCapabilityProfile validated_function_profile_for_mode(const ToshibaFamilyProfile &p,ToshibaHvacMode m){const auto *e=mode_profile_for(p,m);return e?e->functions:ToshibaCapabilityProfile{};}
inline uint8_t validated_fan_options_for_mode(const ToshibaFamilyProfile &p,ToshibaHvacMode m){const auto *e=mode_profile_for(p,m);return e?e->fan_options:FAN_OPTION_NONE;}
inline bool validated_strong_defrost_allowed(const ToshibaFamilyProfile &p,ToshibaHvacMode m){return p.family==ToshibaIndoorUnitFamily::P2KVSG&&m==ToshibaHvacMode::HEAT;}
inline ToshibaFeatureScope feature_scope(ToshibaFeature f){switch(f){
 case FEATURE_VERTICAL_AIRFLOW:case FEATURE_HORIZONTAL_AIRFLOW:case FEATURE_FLOOR:case FEATURE_AIR_OUTLET_SELECT:case FEATURE_FIREPLACE:case FEATURE_HADA_CARE:case FEATURE_PURE:return ToshibaFeatureScope::IDU_LOCAL;
 case FEATURE_COMMON_HVAC:case FEATURE_ECO:case FEATURE_HI_POWER:case FEATURE_COMFORT_SLEEP:case FEATURE_EIGHT_DEG_HEAT:case FEATURE_SLEEP:case FEATURE_COMFORT:return ToshibaFeatureScope::IDU_DEMAND;
 case FEATURE_POWER_SELECT:case FEATURE_OUTDOOR_SILENT:case FEATURE_START_DEFROST:return ToshibaFeatureScope::SHARED_ODU;default:return ToshibaFeatureScope::UNKNOWN;}}
inline const char *feature_scope_to_string(ToshibaFeatureScope s){switch(s){case ToshibaFeatureScope::IDU_LOCAL:return "IDU local";case ToshibaFeatureScope::IDU_DEMAND:return "IDU demand / shared consequence";case ToshibaFeatureScope::SHARED_ODU:return "Shared ODU";default:return "Unknown";}}
} }
