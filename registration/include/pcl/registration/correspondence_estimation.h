/*
 * Software License Agreement (BSD License)
 *
 *  Point Cloud Library (PCL) - www.pointclouds.org
 *  Copyright (c) 2010-2011, Willow Garage, Inc.
 *  Copyright (c) 2012-, Open Perception, Inc.
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the copyright holder(s) nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id$
 *
 */

#pragma once

#include <pcl/common/io.h> // for getFields
#include <pcl/registration/correspondence_types.h>
#include <pcl/search/kdtree.h>
#include <pcl/memory.h>
#include <pcl/pcl_base.h>
#include <pcl/pcl_macros.h>

#include <string>

namespace pcl {
namespace registration {
/** \brief Abstract @b CorrespondenceEstimationBase class.
 * All correspondence estimation methods should inherit from this.
 * \author Radu B. Rusu
 * \ingroup registration
 */
template <typename PointSource, typename PointTarget, typename Scalar = float>
class CorrespondenceEstimationBase : public PCLBase<PointSource> {
public:
  using Ptr =
      shared_ptr<CorrespondenceEstimationBase<PointSource, PointTarget, Scalar>>;
  using ConstPtr =
      shared_ptr<const CorrespondenceEstimationBase<PointSource, PointTarget, Scalar>>;

  // using PCLBase<PointSource>::initCompute;
  using PCLBase<PointSource>::deinitCompute;
  using PCLBase<PointSource>::input_;
  using PCLBase<PointSource>::indices_;
  using PCLBase<PointSource>::setIndices;

  using KdTree = pcl::search::KdTree<PointTarget>;
  using KdTreePtr = typename KdTree::Ptr;
  using KdTreeConstPtr = typename KdTree::ConstPtr;

  using KdTreeReciprocal = pcl::search::KdTree<PointSource>;
  using KdTreeReciprocalPtr = typename KdTreeReciprocal::Ptr;
  using KdTreeReciprocalConstPtr = typename KdTreeReciprocal::ConstPtr;

  using PointCloudSource = pcl::PointCloud<PointSource>;
  using PointCloudSourcePtr = typename PointCloudSource::Ptr;
  using PointCloudSourceConstPtr = typename PointCloudSource::ConstPtr;

  using PointCloudTarget = pcl::PointCloud<PointTarget>;
  using PointCloudTargetPtr = typename PointCloudTarget::Ptr;
  using PointCloudTargetConstPtr = typename PointCloudTarget::ConstPtr;

  using PointRepresentationConstPtr = typename KdTree::PointRepresentationConstPtr;
  using PointRepresentationReciprocalConstPtr =
      typename KdTreeReciprocal::PointRepresentationConstPtr;

  /** \brief Empty constructor. */
  CorrespondenceEstimationBase()
  : corr_name_("CorrespondenceEstimationBase")
  , tree_(new pcl::search::KdTree<PointTarget>)
  , tree_reciprocal_(new pcl::search::KdTree<PointSource>)
  , target_()
  , point_representation_()
  , input_transformed_()
  {}

  /** \brief Empty destructor */
  ~CorrespondenceEstimationBase() override = default;

  /** \brief Provide a pointer to the input source
   * (e.g., the point cloud that we want to align to the target)
   *
   * \param[in] cloud the input point cloud source
   */
  inline void
  setInputSource(const PointCloudSourceConstPtr& cloud)
  {
    source_cloud_updated_ = true;
    PCLBase<PointSource>::setInputCloud(cloud);
    input_fields_ = pcl::getFields<PointSource>();
  }

  /** \brief Get a pointer to the input point cloud dataset target. */
  inline PointCloudSourceConstPtr const
  getInputSource()
  {
    return (input_);
  }

  /** \brief Provide a pointer to the input target
   * (e.g., the point cloud that we want to align the input source to)
   * \param[in] cloud the input point cloud target
   */
  inline void
  setInputTarget(const PointCloudTargetConstPtr& cloud);

  /** \brief Get a pointer to the input point cloud dataset target. */
  inline PointCloudTargetConstPtr const
  getInputTarget()
  {
    return (target_);
  }

  /** \brief Set the number of threads to use.
   * \param nr_threads the number of hardware threads to use (0 sets the value back to
   * automatic)
   */
  void
  setNumberOfThreads(unsigned int nr_threads)
  {
#ifdef _OPENMP
    num_threads_ = nr_threads != 0 ? nr_threads : omp_get_num_procs();
#else
    if (nr_threads != 1) {
      PCL_WARN("OpenMP is not available. Keeping number of threads unchanged at 1\n");
    }
    num_threads_ = 1;
#endif
  }

  /** \brief See if this rejector requires source normals */
  virtual bool
  requiresSourceNormals() const
  {
    return (false);
  }

  /** \brief Abstract method for setting the source normals */
  virtual void
  setSourceNormals(pcl::PCLPointCloud2::ConstPtr /*cloud2*/)
  {
    PCL_WARN("[pcl::registration::%s::setSourceNormals] This class does not require "
             "input source normals\n",
             getClassName().c_str());
  }

  /** \brief See if this rejector requires target normals */
  virtual bool
  requiresTargetNormals() const
  {
    return (false);
  }

  /** \brief Abstract method for setting the target normals */
  virtual void
  setTargetNormals(pcl::PCLPointCloud2::ConstPtr /*cloud2*/)
  {
    PCL_WARN("[pcl::registration::%s::setTargetNormals] This class does not require "
             "input target normals\n",
             getClassName().c_str());
  }

  /** \brief Provide a pointer to the vector of indices that represent the
   * input source point cloud.
   * \param[in] indices a pointer to the vector of indices
   */
  inline void
  setIndicesSource(const IndicesPtr& indices)
  {
    setIndices(indices);
  }

  /** \brief Get a pointer to the vector of indices used for the source dataset. */
  inline IndicesPtr const
  getIndicesSource()
  {
    return (indices_);
  }

  /** \brief Provide a pointer to the vector of indices that represent the input target
   * point cloud. \param[in] indices a pointer to the vector of indices
   */
  inline void
  setIndicesTarget(const IndicesPtr& indices)
  {
    target_cloud_updated_ = true;
    target_indices_ = indices;
  }

  /** \brief Get a pointer to the vector of indices used for the target dataset. */
  inline IndicesPtr const
  getIndicesTarget()
  {
    return (target_indices_);
  }

  /** \brief Provide a pointer to the search object used to find correspondences in
   * the target cloud.
   * \param[in] tree a pointer to the spatial search object.
   * \param[in] force_no_recompute If set to true, this tree will NEVER be
   * recomputed, regardless of calls to setInputTarget. Only use if you are
   * confident that the tree will be set correctly.
   */
  inline void
  setSearchMethodTarget(const KdTreePtr& tree, bool force_no_recompute = false)
  {
    tree_ = tree;
    force_no_recompute_ = force_no_recompute;
    // Since we just set a new tree, we need to check for updates
    target_cloud_updated_ = true;
  }

  /** \brief Get a pointer to the search method used to find correspondences in the
   * target cloud. */
  inline KdTreePtr
  getSearchMethodTarget() const
  {
    return (tree_);
  }

  /** \brief Provide a pointer to the search object used to find correspondences in
   * the source cloud (usually used by reciprocal correspondence finding).
   * \param[in] tree a pointer to the spatial search object.
   * \param[in] force_no_recompute If set to true, this tree will NEVER be
   * recomputed, regardless of calls to setInputSource. Only use if you are
   * extremely confident that the tree will be set correctly.
   */
  inline void
  setSearchMethodSource(const KdTreeReciprocalPtr& tree,
                        bool force_no_recompute = false)
  {
    tree_reciprocal_ = tree;
    force_no_recompute_reciprocal_ = force_no_recompute;
    // Since we just set a new tree, we need to check for updates
    source_cloud_updated_ = true;
  }

  /** \brief Get a pointer to the search method used to find correspondences in the
   * source cloud. */
  inline KdTreeReciprocalPtr
  getSearchMethodSource() const
  {
    return (tree_reciprocal_);
  }

  /** \brief Determine the correspondences between input and target cloud.
   * \param[out] correspondences the found correspondences (index of query point, index
   * of target point, distance) \param[in] max_distance maximum allowed distance between
   * correspondences
   */
  virtual void
  determineCorrespondences(
      pcl::Correspondences& correspondences,
      double max_distance = std::numeric_limits<double>::max()) = 0;

  /** \brief Determine the reciprocal correspondences between input and target cloud.
   * A correspondence is considered reciprocal if both Src_i has Tgt_i as a
   * correspondence, and Tgt_i has Src_i as one.
   *
   * \param[out] correspondences the found correspondences (index of query and target
   * point, distance) \param[in] max_distance maximum allowed distance between
   * correspondences
   */
  virtual void
  determineReciprocalCorrespondences(
      pcl::Correspondences& correspondences,
      double max_distance = std::numeric_limits<double>::max()) = 0;

  /** \brief Provide a boost shared pointer to the PointRepresentation for target cloud
   * to be used when searching for nearest neighbors.
   *
   * \param[in] point_representation the PointRepresentation to be used by the
   * k-D tree for nearest neighbor search
   */
  inline void
  setPointRepresentation(const PointRepresentationConstPtr& point_representation)
  {
    point_representation_ = point_representation;
  }

  /** \brief Provide a boost shared pointer to the PointRepresentation for source cloud
   * to be used when searching for nearest neighbors.
   *
   * \param[in] point_representation the PointRepresentation to be used by the
   * k-D tree for nearest neighbor search
   */
  inline void
  setPointRepresentationReciprocal(
      const PointRepresentationReciprocalConstPtr& point_representation_reciprocal)
  {
    point_representation_reciprocal_ = point_representation_reciprocal;
  }

  /** \brief Clone and cast to CorrespondenceEstimationBase */
  virtual typename CorrespondenceEstimationBase<PointSource, PointTarget, Scalar>::Ptr
  clone() const = 0;

protected:
  /** \brief The correspondence estimation method name. */
  std::string corr_name_;

  /** \brief A pointer to the spatial search object used for the target dataset. */
  KdTreePtr tree_;

  /** \brief A pointer to the spatial search object used for the source dataset. */
  KdTreeReciprocalPtr tree_reciprocal_;

  /** \brief The input point cloud dataset target. */
  PointCloudTargetConstPtr target_;

  /** \brief The target point cloud dataset indices. */
  IndicesPtr target_indices_;

  /** \brief The target point representation used (internal). */
  PointRepresentationConstPtr point_representation_;

  /** \brief The source point representation used (internal). */
  PointRepresentationReciprocalConstPtr point_representation_reciprocal_;

  /** \brief The transformed input source point cloud dataset. */
  PointCloudTargetPtr input_transformed_;

  /** \brief The types of input point fields available. */
  std::vector<pcl::PCLPointField> input_fields_;

  /** \brief Abstract class get name method. */
  inline const std::string&
  getClassName() const
  {
    return (corr_name_);
  }

  /** \brief Internal computation initialization. */
  bool
  initCompute();

  /** \brief Internal computation initialization for reciprocal correspondences. */
  bool
  initComputeReciprocal();

  /** \brief Variable that stores whether we have a new target cloud, meaning we need to
   * pre-process it again. This way, we avoid rebuilding the kd-tree for the target
   * cloud every time the determineCorrespondences () method is called. */
  bool target_cloud_updated_{true};
  /** \brief Variable that stores whether we have a new source cloud, meaning we need to
   * pre-process it again. This way, we avoid rebuilding the reciprocal kd-tree for the
   * source cloud every time the determineCorrespondences () method is called. */
  bool source_cloud_updated_{true};
  /** \brief A flag which, if set, means the tree operating on the target cloud
   * will never be recomputed*/
  bool force_no_recompute_{false};

  /** \brief A flag which, if set, means the tree operating on the source cloud
   * will never be recomputed*/
  bool force_no_recompute_reciprocal_{false};

  unsigned int num_threads_{1};
};

/** \brief @b CorrespondenceEstimation represents the base class for
 * determining correspondences between target and query point
 * sets/features.
 *
 * Code example:
 *
 * \code
 * pcl::PointCloud<pcl::PointXYZRGBA>::Ptr source, target;
 * // ... read or fill in source and target
 * pcl::CorrespondenceEstimation<pcl::PointXYZ, pcl::PointXYZ> est;
 * est.setInputSource (source);
 * est.setInputTarget (target);
 *
 * pcl::Correspondences all_correspondences;
 * // Determine all reciprocal correspondences
 * est.determineReciprocalCorrespondences (all_correspondences);
 * \endcode
 *
 * \author Radu B. Rusu, Michael Dixon, Dirk Holz
 * \ingroup registration
 */
template <typename PointSource, typename PointTarget, typename Scalar = float>
class CorrespondenceEstimation
: public CorrespondenceEstimationBase<PointSource, PointTarget, Scalar> {
public:
  using Ptr = shared_ptr<CorrespondenceEstimation<PointSource, PointTarget, Scalar>>;
  using ConstPtr =
      shared_ptr<const CorrespondenceEstimation<PointSource, PointTarget, Scalar>>;

  using CorrespondenceEstimationBase<PointSource, PointTarget, Scalar>::
      point_representation_;
  using CorrespondenceEstimationBase<PointSource, PointTarget, Scalar>::
      input_transformed_;
  using CorrespondenceEstimationBase<PointSource, PointTarget, Scalar>::tree_;
  using CorrespondenceEstimationBase<PointSource, PointTarget, Scalar>::
      tree_reciprocal_;
  using CorrespondenceEstimationBase<PointSource, PointTarget, Scalar>::target_;
  using CorrespondenceEstimationBase<PointSource, PointTarget, Scalar>::corr_name_;
  using CorrespondenceEstimationBase<PointSource, PointTarget, Scalar>::target_indices_;
  using CorrespondenceEstimationBase<PointSource, PointTarget, Scalar>::getClassName;
  using CorrespondenceEstimationBase<PointSource, PointTarget, Scalar>::initCompute;
  using CorrespondenceEstimationBase<PointSource, PointTarget, Scalar>::
      initComputeReciprocal;
  using CorrespondenceEstimationBase<PointSource, PointTarget, Scalar>::input_;
  using CorrespondenceEstimationBase<PointSource, PointTarget, Scalar>::indices_;
  using CorrespondenceEstimationBase<PointSource, PointTarget, Scalar>::input_fields_;
  using PCLBase<PointSource>::deinitCompute;

  using KdTree =
      typename CorrespondenceEstimationBase<PointSource, PointTarget, Scalar>::KdTree;
  using KdTreePtr = typename CorrespondenceEstimationBase<PointSource,
                                                          PointTarget,
                                                          Scalar>::KdTreePtr;
  using KdTreeConstPtr = typename CorrespondenceEstimationBase<PointSource,
                                                               PointTarget,
                                                               Scalar>::KdTreeConstPtr;
  using KdTreeReciprocal =
      typename CorrespondenceEstimationBase<PointSource, PointTarget, Scalar>::
          KdTreeReciprocal;
  using KdTreeReciprocalPtr =
      typename CorrespondenceEstimationBase<PointSource, PointTarget, Scalar>::
          KdTreeReciprocalPtr;

  using KdTreeReciprocalConstPtr =
      typename CorrespondenceEstimationBase<PointSource, PointTarget, Scalar>::
          KdTreeReciprocalConstPtr;

  using PointCloudSource = pcl::PointCloud<PointSource>;
  using PointCloudSourcePtr = typename PointCloudSource::Ptr;
  using PointCloudSourceConstPtr = typename PointCloudSource::ConstPtr;

  using PointCloudTarget = pcl::PointCloud<PointTarget>;
  using PointCloudTargetPtr = typename PointCloudTarget::Ptr;
  using PointCloudTargetConstPtr = typename PointCloudTarget::ConstPtr;

  using PointRepresentationConstPtr = typename KdTree::PointRepresentationConstPtr;
  using PointRepresentationReciprocalConstPtr =
      typename KdTreeReciprocal::PointRepresentationConstPtr;

  /** \brief Empty constructor. */
  CorrespondenceEstimation() { corr_name_ = "CorrespondenceEstimation"; }

  /** \brief Empty destructor */
  ~CorrespondenceEstimation() override = default;

  /** \brief Determine the correspondences between input and target cloud.
   * \param[out] correspondences the found correspondences (index of query point, index
   * of target point, distance) \param[in] max_distance maximum allowed distance between
   * correspondences
   */
  void
  determineCorrespondences(
      pcl::Correspondences& correspondences,
      double max_distance = std::numeric_limits<double>::max()) override;

  /** \brief Determine the reciprocal correspondences between input and target cloud.
   * A correspondence is considered reciprocal if both Src_i has Tgt_i as a
   * correspondence, and Tgt_i has Src_i as one.
   *
   * \param[out] correspondences the found correspondences (index of query and target
   * point, distance) \param[in] max_distance maximum allowed distance between
   * correspondences
   */
  void
  determineReciprocalCorrespondences(
      pcl::Correspondences& correspondences,
      double max_distance = std::numeric_limits<double>::max()) override;

  /** \brief Clone and cast to CorrespondenceEstimationBase */
  typename CorrespondenceEstimationBase<PointSource, PointTarget, Scalar>::Ptr
  clone() const override
  {
    Ptr copy(new CorrespondenceEstimation<PointSource, PointTarget, Scalar>(*this));
    return (copy);
  }

protected:
  using CorrespondenceEstimationBase<PointSource, PointTarget, Scalar>::num_threads_;
};
} // namespace registration
} // namespace pcl

#include <pcl/registration/impl/correspondence_estimation.hpp>
