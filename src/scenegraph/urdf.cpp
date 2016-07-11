/*
 *  Copyright 2015 Aldebaran. All rights reserved.
 *
 */
#include <almath/scenegraph/urdf.h>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/optional.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <Eigen/Geometry>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <map>
#include <iostream>
#include <almath/scenegraph/urdfeigen.h>

namespace {
using namespace boost::property_tree;
std::string namep(const ptree *pt) { return AL::urdf::name(*pt); }
std::string parent_linkp(const ptree *pt) { return AL::urdf::parent_link(*pt); }
std::string child_linkp(const ptree *pt) { return AL::urdf::child_link(*pt); }

// boost::multiindex does not provide an "at()" member function.
template <typename T, typename K>
typename T::const_iterator checked_find(const T &t, const K &k) {
  typename T::const_iterator it = t.find(k);
  if (it == t.end()) throw std::out_of_range("checked_find");
  return it;
}

template <typename T, typename K>
typename T::iterator checked_find(T &t, const K &k) {
  typename T::iterator it = t.find(k);
  if (it == t.end()) throw std::out_of_range("checked_find");
  return it;
}
}

namespace AL {

namespace urdf {

boost::optional<Array3dTranslator::external_type> Array3dTranslator::get_value(
    const Array3dTranslator::internal_type &str) {
  // str is expected to hold 3 floating point numbers.
  // let split it into 3 strings, then use the usual boost::property_tree
  // translator to convert each substring into a double.
  // In case of failure, return an uninitialized boost::optional, like
  // boost::property_tree does itself.
  boost::tokenizer<> tok(
      str, boost::char_delimiters_separator<char>(false, "", " \t\n\v\f\r"));
  boost::optional<double> d;
  boost::property_tree::translator_between<internal_type, double>::type tr;
  external_type x;  // Array3d
  boost::tokenizer<>::iterator beg = tok.begin();
  size_t i = 0;
  for (; beg != tok.end() && i < 3; ++beg, ++i) {
    d = tr.get_value(*beg);
    if (!d) return boost::optional<external_type>();
    x[i] = *d;
  }
  if (i != 3 || beg != tok.end())
    return boost::optional<external_type>();  // todo: maybe throw
  return boost::optional<external_type>(x);
}

boost::optional<Array3dTranslator::internal_type> Array3dTranslator::put_value(
    const Array3dTranslator::external_type &v) {
  boost::property_tree::translator_between<internal_type, double>::type tr;
  std::ostringstream ss;
  ss << *tr.put_value(v[0]) << " " << *tr.put_value(v[1]) << " "
     << *tr.put_value(v[2]);
  return ss.str();
}

std::string name(const ptree &pt) {
  return pt.get<std::string>("<xmlattr>.name");
}

std::string parent_link(const ptree &pt) {
  return pt.get<std::string>("parent.<xmlattr>.link");
}

std::string child_link(const ptree &pt) {
  return pt.get<std::string>("child.<xmlattr>.link");
}

Pose Pose::inverse() const {
  Pose res;
  if (is_zero(_rpy)) {
    res._xyz = {{-_xyz[0], -_xyz[1], -_xyz[2]}};
  } else {
    Eigen::Matrix3d new_mat =
        Math::eigenQuaternionFromUrdfRpy(_rpy).inverse().matrix();
    res._rpy = Math::urdfRpyFromEigenMatrix3(new_mat);
    if (!is_zero(_xyz)) {
      Eigen::Map<Eigen::Vector3d>(res._xyz.data()) =
          -(new_mat * Eigen::Map<const Eigen::Vector3d>(_xyz.data()));
    }
  }
  return res;
}

Pose operator*(const Pose &lhs, const Pose &rhs) {
  Pose res{lhs.xyz(), {{0, 0, 0}}};
  Eigen::Quaterniond lhs_quat = Math::eigenQuaternionFromUrdfRpy(lhs.rpy());
  if (is_zero(lhs.rpy())) {
    res._rpy = rhs.rpy();
  } else if (is_zero(rhs.rpy())) {
    res._rpy = lhs.rpy();
  } else {
    res._rpy = Math::urdfRpyFromEigenMatrix3(
        (lhs_quat * Math::eigenQuaternionFromUrdfRpy(rhs.rpy())).matrix());
  }
  Eigen::Map<Eigen::Vector3d>(res._xyz.data()) +=
      lhs_quat * Eigen::Map<const Eigen::Vector3d>(rhs._xyz.data());
  return res;
}

// private helper
static Array3d get_pose_child_array3d(const ptree &pt, const char *name) {
  if (boost::optional<const ptree &> child = pt.get_child_optional(name))
    return child->get_value<Array3d>(Array3dTranslator());
  return Array3d{0, 0, 0};
}

Pose Pose::from_ptree(const ptree &pt) {
  return Pose(get_pose_child_array3d(pt, "<xmlattr>.xyz"),
              get_pose_child_array3d(pt, "<xmlattr>.rpy"));
}

Pose Pose::from_ptree(const boost::optional<const ptree &> &pt) {
  return pt ? from_ptree(*pt) : Pose();
}

boost::optional<ptree> Pose::to_ptree() const {
  if (is_zero(xyz()) && is_zero(rpy())) return boost::optional<ptree>();
  ptree res;
  if (!is_zero(xyz()))
    res.put<Array3d>("<xmlattr>.xyz", xyz(), Array3dTranslator());
  if (!is_zero(rpy()))
    res.put<Array3d>("<xmlattr>.rpy", rpy(), Array3dTranslator());
  return res;
}

Joint::Joint(const ptree &pt) : pt(pt) {}
std::string Joint::name() const {
  return pt.get<std::string>("<xmlattr>.name");
}

Joint::Type Joint::type() const {
  const std::string str = pt.get<std::string>("<xmlattr>.type");
  if (str == "revolute") return revolute;
  if (str == "continuous") return continuous;
  if (str == "prismatic") return prismatic;
  if (str == "fixed") return fixed;
  if (str == "floating") return floating;
  if (str == "planar") return planar;
  throw std::runtime_error("unknown joint type");
}

std::string Joint::parent_link() const {
  return pt.get<std::string>("parent.<xmlattr>.link");
}

std::string Joint::child_link() const {
  return pt.get<std::string>("child.<xmlattr>.link");
}

Pose Joint::origin() const {
  return Pose::from_ptree(pt.get_child_optional("origin"));
}

Array3d Joint::axis() const {
  if (boost::optional<const ptree &> child =
          pt.get_child_optional("axis.<xmlattr>.xyz"))
    return child->get_value<Array3d>(Array3dTranslator());
  return Array3d{1, 0, 0};
}

Inertial::Inertial(const ptree &pt) : pt(pt) {}

Pose Inertial::origin() const {
  return Pose::from_ptree(pt.get_child_optional("origin"));
}

double Inertial::mass() const { return pt.get<double>("mass.<xmlattr>.value"); }

#define DEF_INERTIA_GETTER(member)                       \
  double Inertial::member() const {                      \
    return pt.get<double>("inertia.<xmlattr>." #member); \
  }

DEF_INERTIA_GETTER(ixx);
DEF_INERTIA_GETTER(ixy);
DEF_INERTIA_GETTER(ixz);
DEF_INERTIA_GETTER(iyy);
DEF_INERTIA_GETTER(iyz);
DEF_INERTIA_GETTER(izz);

#undef DEF_INERTIA_GETTER

Link::Link(const ptree &pt) : pt(pt) {}

std::string Link::name() const { return pt.get<std::string>("<xmlattr>.name"); }

boost::optional<Inertial> Link::inertial() const {
  if (boost::optional<const ptree &> child = pt.get_child_optional("inertial"))
    return boost::optional<Inertial>(Inertial(*child));
  return boost::optional<Inertial>();
}

namespace detail {

using namespace boost::multi_index;

class UrdfTreeP {
 public:
  struct Name {};
  struct ParentLink {};
  struct ChildLink {};

  typedef boost::multi_index_container<
      ptree *,
      indexed_by<
          sequenced<>,  // used to keep the original joint ordering
          ordered_unique<tag<Name>,
                         global_fun<const ptree *, std::string, &::namep> >,
          ordered_unique<tag<ChildLink>, global_fun<const ptree *, std::string,
                                                    &::child_linkp> >,
          ordered_non_unique<tag<ParentLink>,
                             global_fun<const ptree *, std::string,
                                        &::parent_linkp> > > > Joints;

  typedef std::map<std::string, ptree *> Links;

  typedef std::function<bool(const ptree *)> donotprune_fct;

  ptree &robot;
  Links links;
  Joints joints;
  std::string root_link;
  UrdfTreeP(ptree &pt);  // keeps refs and points to pt elements
  void rm_root_joint();
  void transport_root_link_frame(
      const Pose &pose,
      const donotprune_fct &noprune = [](const ptree *) { return false; });
  void flip_root_link_joint(const std::string &joint_name,
                            bool add_prune_exception);
  void define_as_root_link(const std::string &link_name);
  void walk_joint(UrdfTree::JointConstVisitor &vis,
                  const std::string &link) const;
  void walk_joint(UrdfTree::JointVisitor &vis, const std::string &link);
};

UrdfTreeP::UrdfTreeP(ptree &pt) : robot(pt.get_child("robot")) {
  std::set<std::string> root_links;

  // list all the links
  BOOST_FOREACH (ptree::value_type &child, robot.equal_range("link")) {
    const std::string name = urdf::name(child.second);

    if (!links.insert(Links::value_type(name, &child.second)).second)
      throw std::runtime_error("cannot add link \"" + name +
                               "\": duplicate link name");

    root_links.insert(name);
  }
  if (links.empty()) throw std::runtime_error("this urdf file has no link");

  // list all the joints and check the kinematic tree consistency
  BOOST_FOREACH (ptree::value_type &child, robot.equal_range("joint")) {
    Joint joint(child.second);
    const std::string name = joint.name();

    if (joints.get<Name>().find(name) != joints.get<Name>().end())
      throw std::runtime_error("cannot add joint \"" + name +
                               "\": duplicate joint name");

    if (links.find(joint.parent_link()) == links.end())
      throw std::runtime_error("cannot add joint \"" + name +
                               "\": non-existing parent link \"" +
                               joint.parent_link() + "\"");

    if (links.find(joint.child_link()) == links.end())
      throw std::runtime_error("cannot add joint \"" + name +
                               "\": non-existing child link \"" +
                               joint.child_link() + "\"");

    if (joints.get<ChildLink>().find(joint.child_link()) !=
        joints.get<ChildLink>().end())
      throw std::runtime_error("cannot add joint \"" + name +
                               "\": there is a loop in the kinematic tree");

    joints.push_back(&child.second);
    std::set<std::string>::iterator it = root_links.find(joint.child_link());
    assert(it != root_links.end());
    root_links.erase(it);
    if (root_links.empty())
      throw std::runtime_error("cannot add joint \"" + name +
                               "\": there is a loop in the kinematic tree");
  }
  assert(!root_links.empty());
  if (root_links.size() != 1)
    throw std::runtime_error("kinematic tree has several roots");
  root_link = *root_links.begin();
}

void UrdfTreeP::rm_root_joint() {
  if (joints.get<ParentLink>().count(root_link) != 1) {
    throw std::runtime_error("rm_root_joint: there is not a single root joint");
  }
  const ptree *new_root_joint = *joints.get<ParentLink>().find(root_link);
  joints.get<ParentLink>().erase(root_link);
  links.erase(root_link);
  root_link = Joint(*new_root_joint).child_link();
}

// premultiply the rhs with lhs. So that
// new_rhs = lhs * old_rhs
//
// ilhs shall be the inverse of lhs. Thus, if lhs -== old_rhs, then
// we know that new_rhs == identity, without suffering from numerical noise
// coming from inversion and product.
void premul(const Pose &lhs, const Pose &ilhs, ptree *rhs_parent,
            bool noprune) {
  assert(lhs == ilhs.inverse());
  auto const_rhs_parent = const_cast<const ptree *>(rhs_parent);

  Pose rhs = Pose::from_ptree(const_rhs_parent->get_child_optional("origin"));
  if (rhs == ilhs) {
    if (noprune) {
      // instead of removing the element, empty it.
      // Wrt to URDF, this has the same meaning (identity transform)
      // Keeping the element around is more verbose (bad), but enables to
      // keeps its position in the tree it we want to modify it again
      // (otherwise we would create a new element which would be the last
      // among simblings)
      rhs_parent->put_child("origin", ptree());
    } else {
      rhs_parent->erase("origin");
    }
  } else {
    boost::optional<ptree> new_origin = (lhs * rhs).to_ptree();
    assert(new_origin);
    rhs_parent->put_child("origin", *new_origin);
  }
}

void premul(const Pose &lhs, const Pose &ilhs, ptree::assoc_iterator rhs_parent,
            bool noprune) {
  premul(lhs, ilhs, &(rhs_parent->second), noprune);
}

void UrdfTreeP::transport_root_link_frame(
    const Pose &pose, const std::function<bool(const ptree *)> &noprune) {
  Pose ipose = pose.inverse();
  // relocate the joints origins
  BOOST_FOREACH (ptree *child_joint_pt,
                 joints.get<ParentLink>().equal_range(root_link)) {
    premul(ipose, pose, child_joint_pt, noprune(child_joint_pt));
  }
  ptree *root_link_pt = links.at(root_link);
  for (const std::string &key : {"inertial", "visual", "collision"}) {
    auto range = root_link_pt->equal_range(key);
    for (auto it = range.first; it != range.second; ++it) {
      premul(ipose, pose, it, false);
    }
  }
}

struct flip_joint_links_fctor {
  void operator()(ptree *joint_pt) {
    Joint joint(*joint_pt);
    std::string parent_link = joint.parent_link();
    std::string child_link = joint.child_link();
    joint_pt->put<std::string>("parent.<xmlattr>.link", child_link);
    joint_pt->put<std::string>("child.<xmlattr>.link", parent_link);
  }
};

// If the joint type has an axis, flip it
// (ie. make it go in the opposite direction), so that the joint
// direction is inversed.
void flip_joint_axis(ptree &joint_pt) {
  Joint joint(joint_pt);
  switch (joint.type()) {
    case Joint::Type::revolute:
    case Joint::Type::continuous:
    case Joint::Type::prismatic:
    case Joint::Type::planar: {
      Array3d old_axis = joint.axis();
      Array3d new_axis{{-old_axis[0], -old_axis[1], -old_axis[2]}};
      if (new_axis == Array3d{{1, 0, 0}})
        joint_pt.erase("axis");
      else
        joint_pt.put("axis.<xmlattr>.xyz", new_axis, Array3dTranslator());
      break;
    }
    case Joint::Type::fixed:
    case Joint::Type::floating:
      break;
  };
}

struct joint_has_name_fctor {
  const std::string &name;

  joint_has_name_fctor(const std::string &name) : name(name) {}

  bool operator()(const ptree *joint_pt) {
    return name == Joint(*joint_pt).name();
  }
};

// flip joint named "joint_name"
// The joint must be a child of the root link. As a consequence, flipping it
// will change the kinematic tree root.
//
// We will
// * move the parent link frame
// * inverse the joint's parent and child links
// * inverse the joint axis
//
// This won't change the physical meaning of the kinematic system being
// described.
void UrdfTreeP::flip_root_link_joint(const std::string &joint_name,
                                     bool add_prune_exception) {
  auto joint_it = checked_find(joints.get<Name>(), joint_name);
  ptree *joint_pt = *joint_it;
  Joint joint(*joint_pt);
  assert(joint.parent_link() == root_link);
  // The root link frame can be defined anywhere. But this is not the
  // case for other links, whose frame is defined by their parent joint,
  // and must have its origin on the parent joint axis.
  //
  // Since the current root link won't be a root link anymore, we need to
  // move its frame on its future parent joint axis.
  // So, let move the the root link frame so that is is identical to the
  // future parent joint when this joint is at the zero pose.
  // Note: maybe we should instead do a pure translation?
  boost::optional<const ptree &> opt_origin =
      const_cast<const ptree *>(joint_pt)->get_child_optional("origin");
  if (opt_origin) {
    Pose origin = Pose::from_ptree(opt_origin);
    if (add_prune_exception) {
      UrdfTreeP::transport_root_link_frame(origin,
                                           joint_has_name_fctor(joint_name));
    } else {
      UrdfTreeP::transport_root_link_frame(origin);
    }
  }
  // update kinematic tree
  bool ok = joints.get<Name>().modify(joint_it, flip_joint_links_fctor());
  assert(ok);
  root_link = joint.parent_link();
  flip_joint_axis(*joint_pt);
}

void UrdfTreeP::define_as_root_link(const std::string &link_name) {
  std::vector<std::string> joints_path;
  std::string cur_link_name = link_name;
  while (cur_link_name != root_link) {
    Joint joint(**checked_find(joints.get<ChildLink>(), cur_link_name));
    if ((joint.type() == Joint::floating) || (joint.type() == Joint::planar))
      throw std::runtime_error(
          "define_as_root_link: there is a multi-dof"
          " joint between the old and the new root");
    joints_path.push_back(joint.name());

    cur_link_name = joint.parent_link();
  }
  for (auto it = joints_path.rbegin(); it != joints_path.rend(); ++it) {
    bool add_prune_exception = it + 1 != joints_path.rend();
    flip_root_link_joint(*it, add_prune_exception);
  }
}

void UrdfTreeP::walk_joint(UrdfTree::JointConstVisitor &vis,
                           const std::string &link) const {
  BOOST_FOREACH (const ptree *joint,
                 joints.get<ParentLink>().equal_range(link)) {
    if (vis.discover(*joint)) walk_joint(vis, Joint(*joint).child_link());
    vis.finish(*joint);
  }
}

void UrdfTreeP::walk_joint(UrdfTree::JointVisitor &vis,
                           const std::string &link) {
  BOOST_FOREACH (ptree *joint, joints.get<ParentLink>().equal_range(link)) {
    if (vis.discover(*joint)) walk_joint(vis, Joint(*joint).child_link());
    vis.finish(*joint);
  }
}

}  // end of namespace AL::urdf::detail

UrdfTree::UrdfTree(ptree &pt) : _p(new detail::UrdfTreeP(pt)) {}
UrdfTree::~UrdfTree() {}

const ptree &UrdfTree::link(const std::string &name) const {
  return *_p->links.at(name);
}
ptree &UrdfTree::link(const std::string &name) { return *_p->links.at(name); }
const ptree &UrdfTree::joint(const std::string &name) const {
  return **checked_find(_p->joints.get<detail::UrdfTreeP::Name>(), name);
}
ptree &UrdfTree::joint(const std::string &name) {
  return **checked_find(_p->joints.get<detail::UrdfTreeP::Name>(), name);
}

const std::string &UrdfTree::root_link() const { return _p->root_link; }

void UrdfTree::rm_root_joint() { _p->rm_root_joint(); }

void UrdfTree::transport_root_link_frame(const Pose &pose) {
  _p->transport_root_link_frame(pose);
}

void UrdfTree::define_as_root_link(const std::string &name) {
  _p->define_as_root_link(name);
}

void UrdfTree::traverse_joints(JointConstVisitor &vis) const {
  _p->walk_joint(vis, _p->root_link);
}

void UrdfTree::traverse_joints(JointVisitor &vis) {
  _p->walk_joint(vis, _p->root_link);
}

UrdfDotPrinterVisitor::UrdfDotPrinterVisitor(std::ostream &os)
    : tab('\t'), depth(1), do_indent(false), os(os) {}

bool UrdfDotPrinterVisitor::discover(const ptree &joint_pt) {
  Joint joint(joint_pt);
  os << (do_indent ? std::string(depth, tab) : "") << joint.parent_link()
     << " -> " << joint.child_link() << " [label=" << joint.name() << "];\n";
  ++depth;
  return true;
}

void UrdfDotPrinterVisitor::finish(const ptree &joint) { --depth; }

// internal helper
void makeJointFixed(ptree &joint_pt) {
  joint_pt.put("<xmlattr>.type", "fixed");
  joint_pt.erase("axis");
}

void makeJointFixed(UrdfTree &parser, const std::string &name) {
  makeJointFixed(parser.joint(name));
}

void makeJointFloating(UrdfTree &parser, const std::string &name) {
  ptree &joint = parser.joint(name);
  joint.put("<xmlattr>.type", "floating");
  joint.erase("axis");
}

// internal helper class
class MakeContinuousJointsFixedVisitor : public JointVisitor {
 public:
  MakeContinuousJointsFixedVisitor() {}
  std::vector<std::string> names;
  bool discover(ptree &joint_pt) {
    if (Joint(joint_pt).type() == Joint::continuous) {
      makeJointFixed(joint_pt);
      names.push_back(Joint(joint_pt).name());
    }
    return true;
  }
};

std::vector<std::string> makeContinuousJointsFixed(UrdfTree &parser) {
  MakeContinuousJointsFixedVisitor vis;
  parser.traverse_joints(vis);
  return vis.names;
}

// internal helper overload
void squashJointMass(UrdfTree &parser, ptree &joint_pt) {
  typedef Eigen::Matrix3d Matrix3;
  typedef Eigen::Vector3d Vector3;
  typedef Eigen::Transform<double, 3, Eigen::AffineCompact, Eigen::DontAlign>
      EPose;

  Joint joint(joint_pt);
  ptree &child_pt = parser.link(joint.child_link());
  Link child(child_pt);
  boost::optional<Inertial> child_inertial = child.inertial();
  if (!child_inertial) return;  // nothing to squash

  // pose ot the child inertial in the parent frame.
  const EPose child_inertial_pose =
      Math::toEigenTransform(joint.origin()) *
      Math::toEigenTransform(child_inertial->origin());

  EPose new_inertial_pose;
  ptree new_inertial;

  ptree &parent_pt = parser.link(joint.parent_link());
  Link parent(parent_pt);
  boost::optional<Inertial> parent_inertial = parent.inertial();
  if (!parent_inertial) {
    // we can copy the child inertial as-is, except for the pose which
    // needs to get updated.
    new_inertial = child_inertial->pt;
    new_inertial_pose = child_inertial_pose;
  } else {
    const EPose parent_inertial_pose =
        Math::toEigenTransform(parent_inertial->origin());
    double new_mass;
    Matrix3 new_inertia;

    Math::squashInertial(parent_inertial->mass(), parent_inertial_pose,
                         Math::toEigenMatrix3(*parent_inertial),
                         child_inertial->mass(), child_inertial_pose,
                         Math::toEigenMatrix3(*child_inertial), new_mass,
                         new_inertial_pose, new_inertia);
    new_inertial.put("mass.<xmlattr>.value", new_mass);
    new_inertial.put("inertia.<xmlattr>.ixx", new_inertia(0, 0));
    new_inertial.put("inertia.<xmlattr>.ixy", new_inertia(0, 1));
    new_inertial.put("inertia.<xmlattr>.ixz", new_inertia(0, 2));
    new_inertial.put("inertia.<xmlattr>.iyy", new_inertia(1, 1));
    new_inertial.put("inertia.<xmlattr>.iyz", new_inertia(1, 2));
    new_inertial.put("inertia.<xmlattr>.izz", new_inertia(2, 2));
  }
  if (new_inertial_pose.translation() != Vector3::Zero()) {
    Array3d xyz = {{new_inertial_pose.translation()[0],
                    new_inertial_pose.translation()[1],
                    new_inertial_pose.translation()[2]}};
    new_inertial.put("origin.<xmlattr>.xyz", xyz, urdf::Array3dTranslator());
  }
  if (new_inertial_pose.linear() != Matrix3::Identity()) {
    new_inertial.put("origin.<xmlattr>.rpy",
                     Math::urdfRpyFromEigenMatrix3(new_inertial_pose.linear()),
                     urdf::Array3dTranslator());
  }
  parent_pt.put_child("inertial", new_inertial);
  child_pt.erase("inertial");
}

void squashJointMass(UrdfTree &parser, const std::string &name) {
  squashJointMass(parser, parser.joint(name));
}

// internal helper class
class SquashFixedJointsMassVisitor : public JointVisitor {
 public:
  SquashFixedJointsMassVisitor(UrdfTree &parser) : parser(parser) {}
  void finish(ptree &joint_pt) {
    if (Joint(joint_pt).type() != Joint::fixed) return;
    squashJointMass(parser, joint_pt);
  }

 private:
  UrdfTree &parser;
};

void squashFixedJointsMass(UrdfTree &parser) {
  urdf::SquashFixedJointsMassVisitor vis(parser);
  parser.traverse_joints(vis);
}

// internal helper class
class MakeMasslessJointsFixedVisitor : public JointVisitor {
 public:
  MakeMasslessJointsFixedVisitor(UrdfTree &parser) : parser(parser) {}
  std::vector<std::string> names;
  void finish(ptree &joint_pt) {
    Joint joint(joint_pt);
    if (joint.type() == Joint::fixed) {
      squashJointMass(parser, joint_pt);
    } else if (!Link(parser.link(joint.child_link())).inertial()) {
      makeJointFixed(joint_pt);
      names.push_back(joint.name());
      // note: no need to call squashJointMass, we know the child link has no
      // mass
    }
  }

 private:
  UrdfTree &parser;
};

std::vector<std::string> makeMasslessJointsFixed(UrdfTree &parser) {
  urdf::MakeMasslessJointsFixedVisitor vis(parser);
  parser.traverse_joints(vis);
  return vis.names;
}
}
}