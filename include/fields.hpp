// Copyright (C) 2023 Marrony Neris

#ifndef INCLUDE_FIELDS_HPP_
#define INCLUDE_FIELDS_HPP_

#include <string>
#include <parser.hpp>

namespace alpaca {
namespace fields {

// boolean fields
constexpr const parser::field<bool> connected_f = { "Connected" };
constexpr const parser::field<bool> doesrefraction_f = { "DoesRefraction" };
constexpr const parser::field<bool> tracking_f = { "Tracking" };

// float fields
constexpr const parser::field<float> altitude_f = { "Altitude" };
constexpr const parser::field<float> azimuth_f = { "Azimuth" };
constexpr const parser::field<float> rightascension_f = { "RightAscension" };
constexpr const parser::field<float> rightascensionrate_f = { "RightAscensionRate" };
constexpr const parser::field<float> declination_f = { "Declination" };
constexpr const parser::field<float> declinationrate_f = { "DeclinationRate" };
constexpr const parser::field<float> guideratedeclination_f = { "GuideRateDeclination" };
constexpr const parser::field<float> guideraterightascension_f = {
  "GuideRateRightAscension"
};
constexpr const parser::field<float> siteelevation_f = { "SiteElevation" };
constexpr const parser::field<float> sitelatitude_f = { "SiteLatitude" };
constexpr const parser::field<float> sitelongitude_f = { "SiteLongitude" };
constexpr const parser::field<float> targetdeclination_f = { "TargetDeclination" };
constexpr const parser::field<float> targetrightascension_f = { "TargetRightAscension" };
constexpr const parser::field<float> rate_f = { "Rate" };

// int fields
constexpr const parser::field<int> axis_f = { "Axis" };
constexpr const parser::field<int> direction_f = { "Direction" };
constexpr const parser::field<int> duration_f = { "Duration" };
constexpr const parser::field<int> sideofpier_f = { "SideOfPier" };
constexpr const parser::field<int> slewsettletime_f = { "SlewSettleTime" };
constexpr const parser::field<int> trackingrate_f = { "TrackingRate" };

// string fields
constexpr const parser::field<std::string_view> utcdate_f = { "UTCDate" };

}  // namespace fields
}  // namespace alpaca

#endif  // INCLUDE_FIELDS_HPP_
