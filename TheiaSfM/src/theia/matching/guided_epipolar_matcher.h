// Copyright (C) 2016 The Regents of the University of California (Regents).
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//
//     * Neither the name of The Regents or University of California nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Please contact the author of this library if you have any questions.
// Author: Chris Sweeney (cmsweeney@cs.ucsb.edu)

#ifndef THEIA_MATCHING_GUIDED_EPIPOLAR_MATCHER_H_
#define THEIA_MATCHING_GUIDED_EPIPOLAR_MATCHER_H_

#include <Eigen/Core>
#include <functional>
#include <vector>
#include <unordered_map>
#include <utility>

#include "theia/matching/indexed_feature_match.h"
#include "theia/matching/keypoints_and_descriptors.h"
#include "theia/sfm/camera/camera.h"
#include "theia/util/hash.h"

namespace theia
{
class GuidedEpipolarMatcher
{
public:
  struct Options
  {
    // For guided matching, features that are closer than this threshold to the
    // epipolar line will be considered for matching.
    double guided_matching_max_distance_pixels = 2.0;

    // For matching, only keep matches that pass the lowes ratio test where the
    // nearest neighbor's match distance is less than lowes_ratio * the second
    // nearest neighbor's descriptor distance.
    double lowes_ratio = 0.8;
  };

  GuidedEpipolarMatcher(const Options& options, const Camera& camera1, const Camera& camera2,
                        const KeypointsAndDescriptors& features1, const KeypointsAndDescriptors& features2)
    : options_(options), camera1_(camera1), camera2_(camera2), features1_(features1), features2_(features2)
  {
  }

  // Find matches using a guided search strategy. Valid matches are appended to
  // the matches vector, and only features that do not contain a match are used
  // for guided matching.
  bool GetMatches(std::vector<IndexedFeatureMatch>* matches);

private:
  // This helper class provides quick and easy access to the image grids that
  // are used to rapidly find features near epipolar lines.
  class ImageGrid
  {
  public:
    ImageGrid(const double cell_size, const double cell_offset_x, const double cell_offset_y);

    // Add a feature the the grid and assign it to a cell.
    void AddFeature(const int feature_index, const double x, const double y);

    // Retrieve the closest cell center to the point.
    void GetClosestGridCenter(const double x, const double y, Eigen::Vector2i* grid_center);

    // Get all features that lie within the cell specified by the center.
    void GetFeaturesFromCell(const Eigen::Vector2i& cell_center, std::vector<int>* feature_indices);

  private:
    // A container for the grid cells. The mapping is such the the cell center
    // contains all keypoints in the cell.
    std::unordered_map<Eigen::Vector2i, std::vector<int>> cells_;
    double cell_size_, cell_offset_x_, cell_offset_y_;
  };

  // Holds a group of features with similar epiplines as a single epiline.
  struct EpilineGroup
  {
    std::vector<Eigen::Vector2d> endpoints;
    std::vector<int> features;
  };

  // Creates the grid structure for the fast epipolar lookup.
  bool Initialize(const std::vector<IndexedFeatureMatch>& matches);

  // Groups similar epipolar lines into groups so that the computational
  // workload may be reduced.
  void GroupEpipolarLines(std::vector<EpilineGroup>* epiline_groups);

  // Finds all features near a given epipolar line.
  void FindFeaturesNearEpipolarLines(const EpilineGroup& epiline_group, std::vector<int>* candidate_keypoint_indices);

  // Finds the intersection of an epipolar line with the bounding box of the
  // features.
  void FindEpipolarLineIntersection(const Eigen::Vector3d& epipolar_line, std::vector<Eigen::Vector2d>* lines);

  // Computes a fundamental matrix from the cameras.
  Eigen::Matrix3d ComputeFundamentalMatrix();

  // Finds the closest grid cell among all image grids and returns the keypoints
  // in that cell.
  void FindClosestCellAndKeypoints(const Eigen::Vector2d& point, std::vector<int>* new_keypoints);

  // Given the set of query descriptors (in features1) and the candidate matches
  // (in features2), return the top 2 nearest neighbor distances and indices
  // where the index is the index in features2 of the match. The format is
  // nn_distances[query_feature_index][nn_number] where nn_number == 0 is the
  // closest neighbor by descriptor distance.
  void FindKNearestNeighbors(const std::vector<int>& query_feature_indices,
                             const std::vector<int>& candidate_feature_indices,
                             std::vector<std::vector<float>>* nn_distances, std::vector<std::vector<int>>* nn_indices);

  const Options options_;
  const Camera &camera1_, camera2_;
  const KeypointsAndDescriptors &features1_, features2_;

  Eigen::Vector2d top_left_, bottom_right_;
  std::vector<ImageGrid> image_grids_;
  std::unordered_set<int> matched_features1_, matched_features2_;
};

}  // namespace theia

#endif  // THEIA_MATCHING_GUIDED_EPIPOLAR_MATCHER_H_
