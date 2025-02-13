// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/learning/impl/random_tree.h"

namespace media {
namespace learning {

RandomTree::TreeNode::~TreeNode() = default;
RandomTree::Split::Split() = default;
RandomTree::Split::Split(int index) : split_index(index) {}
RandomTree::Split::Split(Split&& rhs) = default;
RandomTree::Split::~Split() = default;
RandomTree::Split& RandomTree::Split::operator=(Split&& rhs) = default;
RandomTree::Split::BranchInfo::BranchInfo() = default;
RandomTree::Split::BranchInfo::BranchInfo(const BranchInfo& rhs) = default;
RandomTree::Split::BranchInfo::~BranchInfo() = default;

struct InteriorNode : public RandomTree::TreeNode {
  InteriorNode(int split_index) : split_index_(split_index) {}

  // TreeNode
  TargetDistribution* ComputeDistribution(
      const FeatureVector& features) override {
    auto iter = children_.find(features[split_index_]);
    // If we've never seen this feature value, then make no prediction.
    if (iter == children_.end())
      return nullptr;

    return iter->second->ComputeDistribution(features);
  }

  // Add |child| has the node for feature value |v|.
  void AddChild(FeatureValue v, std::unique_ptr<TreeNode> child) {
    DCHECK_EQ(children_.count(v), 0u);
    children_.emplace(v, std::move(child));
  }

 private:
  // Feature value that we split on.
  int split_index_ = -1;
  std::map<FeatureValue, std::unique_ptr<TreeNode>> children_;
};

struct LeafNode : public RandomTree::TreeNode {
  LeafNode(const TrainingData& training_data) {
    for (const TrainingExample* example : training_data)
      distribution_[example->target_value]++;
  }

  // TreeNode
  TargetDistribution* ComputeDistribution(const FeatureVector&) override {
    return &distribution_;
  }

 private:
  TargetDistribution distribution_;
};

RandomTree::RandomTree() = default;

RandomTree::~RandomTree() = default;

void RandomTree::Train(const TrainingData& training_data) {
  root_ = nullptr;
  if (training_data.empty())
    return;

  root_ = Build(training_data, FeatureSet());
}

const RandomTree::TreeNode::TargetDistribution*
RandomTree::ComputeDistributionForTesting(const FeatureVector& instance) {
  if (!root_)
    return nullptr;
  return root_->ComputeDistribution(instance);
}

std::unique_ptr<RandomTree::TreeNode> RandomTree::Build(
    const TrainingData& training_data,
    const FeatureSet& used_set) {
  DCHECK(training_data.size());

  // TODO(liberato): Does it help if we refuse to split without an info gain?
  Split best_potential_split;

  // Select the feature subset to consider at this leaf.
  // TODO(liberato): This should select a subset, which is why it's not merged
  // with the loop below.
  FeatureSet feature_candidates;
  for (size_t i = 0; i < training_data[0]->features.size(); i++) {
    if (used_set.find(i) != used_set.end())
      continue;
    feature_candidates.insert(i);
  }

  // Find the best split among the candidates that we have.
  for (int i : feature_candidates) {
    Split potential_split = ConstructSplit(training_data, i);
    if (potential_split.nats_remaining < best_potential_split.nats_remaining) {
      best_potential_split = std::move(potential_split);
    }
  }

  // Note that we can have a split with no index (i.e., no features left, or no
  // feature was an improvement in nats), or with a single index (had features,
  // but all had the same value).  Either way, we should end up with a leaf.
  if (best_potential_split.branch_infos.size() < 2) {
    // Stop when there is no more tree.
    return std::make_unique<LeafNode>(training_data);
  }

  // Build an interior node
  std::unique_ptr<InteriorNode> node =
      std::make_unique<InteriorNode>(best_potential_split.split_index);

  // Don't let the subtree use this feature.
  FeatureSet new_used_set(used_set);
  new_used_set.insert(best_potential_split.split_index);

  for (auto& branch_iter : best_potential_split.branch_infos) {
    node->AddChild(branch_iter.first,
                   Build(branch_iter.second.training_data, new_used_set));
  }

  return node;
}

RandomTree::Split RandomTree::ConstructSplit(const TrainingData& training_data,
                                             int index) {
  // We should not be given a training set of size 0, since there's no need to
  // check an empty split.
  DCHECK_GT(training_data.size(), 0u);

  Split split(index);

  // Find the split's feature values and construct the training set for each.
  // I think we want to iterate on the underlying vector, and look up the int in
  // the training data directly.
  for (const TrainingExample* example : training_data) {
    // Get the value of the |index|-th feature for
    FeatureValue v_i = example->features[split.split_index];

    // Add |v_i| to the right training set.
    Split::BranchInfo& branch_info = split.branch_infos[v_i];
    branch_info.training_data.push_back(example);
    branch_info.class_counts[example->target_value]++;
  }

  // Compute the nats given that we're at this node.
  split.nats_remaining = 0;
  for (auto& info_iter : split.branch_infos) {
    Split::BranchInfo& branch_info = info_iter.second;

    const int total_counts = branch_info.training_data.size();
    for (auto& iter : branch_info.class_counts) {
      double p = ((double)iter.second) / total_counts;
      split.nats_remaining -= p * log(p);
    }
  }

  return split;
}

}  // namespace learning
}  // namespace media
