/******************************************************************************
 * Copyright 2017 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

/**
 * @file
 **/

#include "modules/planning/lattice/util/reference_line_matcher.h"

#include <algorithm>
#include <vector>
#include <cmath>

#include "modules/planning/math/hermite_spline.h"
#include "modules/planning/common/planning_util.h"
#include "modules/common/math/integral.h"
#include "modules/common/math/search.h"
#include "modules/common/math/math_utils.h"

namespace apollo {
namespace planning {

using apollo::common::PathPoint;
using apollo::common::math::IntegrateByGaussLegendre;
using apollo::planning::util::InterpolateUsingLinearApproximation;
using apollo::planning::util::interpolate;
using apollo::common::math::GoldenSectionSearch;

PathPoint ReferenceLineMatcher::MatchToReferenceLine(
    const std::vector<PathPoint>& reference_line, const double x,
    const double y) {
  CHECK_GT(reference_line.size(), 0);

  auto func_distance_square =
      [](const PathPoint& point, const double x, const double y) {
    double dx = point.x() - x;
    double dy = point.y() - y;
    return dx * dx + dy * dy;
  };

  double distance_min = func_distance_square(reference_line.front(), x, y);
  std::size_t index_min = 0;

  for (std::size_t i = 1; i < reference_line.size(); ++i) {
    double distance_temp = func_distance_square(reference_line[i], x, y);
    if (distance_temp < distance_min) {
      distance_min = distance_temp;
      index_min = i;
    }
  }

  std::size_t index_start = (index_min == 0) ? index_min : index_min - 1;
  std::size_t index_end =
      (index_min + 1 == reference_line.size()) ? index_min : index_min + 1;

  if (index_start == index_end) {
    return reference_line[index_start];
  }

  return FindProjectionPoint(reference_line[index_start],
                             reference_line[index_end], x, y);
}

std::pair<double, double> ReferenceLineMatcher::GetReferenceLineCoordinate(
    const std::vector<common::PathPoint>& reference_line, const double x,
    const double y) {
  auto matched_path_point = MatchToReferenceLine(reference_line, x, y);

  std::pair<double, double> relative_coordinate;
  relative_coordinate.first = matched_path_point.s();
  relative_coordinate.second =
      std::hypot(matched_path_point.x() - x, matched_path_point.y() - y);
  return relative_coordinate;
}


PathPoint ReferenceLineMatcher::MatchToReferenceLine(
    const std::vector<PathPoint>& reference_line, const double s) {
  auto comp =
      [](const PathPoint& point, const double s) { return point.s() < s; };

  auto it_lower =
      std::lower_bound(reference_line.begin(), reference_line.end(), s, comp);
  if (it_lower == reference_line.begin()) {
    return reference_line.front();
  } else if (it_lower == reference_line.end()) {
    return reference_line.back();
  }

  // interpolate between it_lower - 1 and it_lower
  // return interpolate(*(it_lower - 1), *it_lower, s);
  return InterpolateUsingLinearApproximation(*(it_lower - 1), *it_lower, s);
}

PathPoint ReferenceLineMatcher::FindProjectionPoint(const PathPoint& p0,
                                                    const PathPoint& p1,
                                                    const double x,
                                                    const double y) {
  double v0x = x - p0.x();
  double v0y = y - p0.y();

  double v1x = p1.x() - p0.x();
  double v1y = p1.y() - p0.y();

  double v1_norm = std::sqrt(v1x * v1x + v1y * v1y);
  double dot = v0x * v1x + v0y * v1y;

  double delta_s = dot / v1_norm;
  return InterpolateUsingLinearApproximation(p0, p1, p0.s() + delta_s);
}

}  // namespace planning
}  // namespace apollo
