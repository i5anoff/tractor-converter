
        /*******************************************************
        *                                                      *
        *  volInt.c                                            *
        *                                                      *
        *  This code computes volume integrals needed for      *
        *  determining mass properties of polyhedral bodies.   *
        *                                                      *
        *  For more information, see the accompanying README   *
        *  file, and the paper                                 *
        *                                                      *
        *  Brian Mirtich, "Fast and Accurate Computation of    *
        *  Polyhedral Mass Properties," journal of graphics    *
        *  tools, volume 1, number 1, 1996.                    *
        *                                                      *
        *  This source code is public domain, and may be used  *
        *  in any way, shape or form, free of charge.          *
        *                                                      *
        *  Copyright 1995 by Brian Mirtich                     *
        *                                                      *
        *  mirtich@cs.berkeley.edu                             *
        *  http://www.cs.berkeley.edu/~mirtich                 *
        *                                                      *
        *******************************************************/

/*
        Revision history

        26 Jan 1996     Program creation.

         3 Aug 1996     Corrected bug arising when polyhedron density
                        is not 1.0.  Changes confined to function main().
                        Thanks to Zoran Popovic for catching this one.

        27 May 1997     Corrected sign error in translation of inertia
                        product terms to center of mass frame.  Changes 
                        confined to function main().  Thanks to 
                        Chris Hecker.
*/



// Modified for tractor_converter.



#include "volInt.hpp"



namespace volInt{



double degrees_to_radians(double degrees)
{
  return degrees * (M_PI / 180.0);
}



//// VANGERS SOURCE
//// How angle conversions work in Vangers code.
//#define M_PI 3.14159265358979323846
//
//Pi_len = 11
//const int Pi = 1 << Pi_len;
//2048
//
//#define GTOR(x) (double(x) * (M_PI / double(Pi)))
//#define RTOG(x) (round(x *   (double(Pi) / M_PI)))

double sicher_angle_to_radians(int sicher_angle)
{
  return static_cast<double>(sicher_angle) *
         (M_PI / static_cast<double>(sicher_angle_Pi));
}

int radians_to_sicher_angle(double radians)
{
  return std::round(radians * (static_cast<double>(sicher_angle_Pi) / M_PI));
}



//// VANGERS SOURCE
//// How rotation works in Vangers code.
//inline Vector Vector::operator*(const DBM &m) const
//{
//  return
//    Vector(
//      round(m.a[0] * x + m.a[1] * y + m.a[2] * z),
//      round(m.a[3] * x + m.a[4] * y + m.a[5] * z),
//      round(m.a[6] * x + m.a[7] * y + m.a[8] * z));
//}
//
//inline DBM::DBM(int angle, int axis)
//{
////------ Calculate Matrix for ROTATE point an angle ------
//  double calpha = Cos(angle);
//  double salpha = Sin(angle);
//  switch(axis){
//    case Z_AXIS:
//      a[0] =  calpha; a[1] = -salpha; a[2] =  0;
//      a[3] =  salpha; a[4] =  calpha; a[5] =  0;
//      a[6] =  0;      a[7] =  0;      a[8] =  1;
//      break;
//    case X_AXIS:
//      a[0] =  1;      a[1] =  0;      a[2] =  0;
//      a[3] =  0;      a[4] =  calpha; a[5] = -salpha;
//      a[6] =  0;      a[7] =  salpha; a[8] =  calpha;
//      break;
//    case Y_AXIS:
//      a[0] =  calpha; a[1] =  0;      a[2] =  salpha;
//      a[3] =  0;      a[4] =  1;      a[5] =  0;
//      a[6] = -salpha; a[7] =  0;      a[8] =  calpha;
//      break;
//    }
//}

void rotate_point_by_axis(
  std::vector<double> &point_arg,
  double angle_sin,
  double angle_cos,
  rotation_axis axis)
{
  std::vector<double> point_orig = point_arg;

  const std::vector<std::size_t> &axes =
    axes_by_plane_continuous[static_cast<std::size_t>(axis)];
  point_arg[axes[0]] =
    angle_cos * point_orig[axes[0]] - angle_sin * point_orig[axes[1]];
  point_arg[axes[1]] =
    angle_sin * point_orig[axes[0]] + angle_cos * point_orig[axes[1]];
}

void rotate_point_by_axis(std::vector<double> &point_arg,
                          double angle,
                          rotation_axis axis)
{
  if(angle == 0.0)
  {
    return;
  }
  double angle_sin = std::sin(angle);
  double angle_cos = std::cos(angle);
  rotate_point_by_axis(point_arg, angle_sin, angle_cos, axis);
}





std::vector<double> vector_scale(double norm, const std::vector<double> &vec)
{
  double vec_length = vector_length(vec);
  double s;
  if(vec_length == 0.0)
  {
    s = 0.0;
  }
  else
  {
    s = norm / vec_length;
  }

  std::vector<double> ret(axes_num, 0.0);
  for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
  {
    ret[cur_coord] = vec[cur_coord] * s;
  }

  return ret;
}



void vector_scale_self(double norm, std::vector<double> &vec)
{
  double vec_length = vector_length(vec);
  double s;
  if(vec_length == 0.0)
  {
    s = 0.0;
  }
  else
  {
    s = norm / vec_length;
  }

  for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
  {
    vec[cur_coord] *= s;
  }
}



void vector_invert_self(std::vector<double> &vec)
{
  for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
  {
    vec[cur_coord] = -vec[cur_coord];
  }
}



std::vector<double> vector_plus(const std::vector<double> &first,
                                const std::vector<double> &second)
{
  std::vector<double> ret(axes_num, 0.0);
  for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
  {
    ret[cur_coord] = first[cur_coord] + second[cur_coord];
  }
  return ret;
}



void vector_plus_self(std::vector<double> &first,
                      const std::vector<double> &second)
{
  for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
  {
    first[cur_coord] += second[cur_coord];
  }
}



std::vector<double> vector_minus(const std::vector<double> &first,
                                 const std::vector<double> &second)
{
  std::vector<double> ret(axes_num, 0.0);
  for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
  {
    ret[cur_coord] = first[cur_coord] - second[cur_coord];
  }
  return ret;
}



void vector_minus_self(std::vector<double> &first,
                       const std::vector<double> &second)
{
  for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
  {
    first[cur_coord] -= second[cur_coord];
  }
}



std::vector<double> vector_multiply(const std::vector<double> &first,
                                    const std::vector<double> &second)
{
  std::vector<double> ret(axes_num, 0.0);
  for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
  {
    ret[cur_coord] = first[cur_coord] * second[cur_coord];
  }
  return ret;
}



void vector_multiply_self(std::vector<double> &first,
                          const std::vector<double> &second)
{
  for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
  {
    first[cur_coord] *= second[cur_coord];
  }
}



std::vector<double> vector_multiply(const std::vector<double> &vec,
                                    double num)
{
  std::vector<double> ret(axes_num, 0.0);
  for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
  {
    ret[cur_coord] = vec[cur_coord] * num;
  }
  return ret;
}



void vector_multiply_self(std::vector<double> &vec,
                          double num)
{
  for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
  {
    vec[cur_coord] *= num;
  }
}



std::vector<double> vector_divide(const std::vector<double> &first,
                                  const std::vector<double> &second)
{
  std::vector<double> ret(axes_num, 0.0);
  for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
  {
    ret[cur_coord] = first[cur_coord] / second[cur_coord];
  }
  return ret;
}



void vector_divide_self(std::vector<double> &first,
                        const std::vector<double> &second)
{
  for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
  {
    first[cur_coord] /= second[cur_coord];
  }
}



std::vector<double> vector_divide(const std::vector<double> &vec,
                                  double num)
{
  std::vector<double> ret(axes_num, 0.0);
  for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
  {
    ret[cur_coord] = vec[cur_coord] / num;
  }
  return ret;
}



void vector_divide_self(std::vector<double> &vec,
                        double num)
{
  for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
  {
    vec[cur_coord] /= num;
  }
}



double vector_length(const std::vector<double> &vec)
{
  double length = 0.0;
  for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
  {
    length += vec[cur_coord] * vec[cur_coord];
  }
  return std::sqrt(length);
}



double vector_length_between(
  const std::vector<double> &first,
  const std::vector<double> &second)
{
  std::vector<double> tmp_vec = vector_minus(first, second);
  return vector_length(tmp_vec);
}



double vector_dot_product(const std::vector<double> &first,
                          const std::vector<double> &second)
{
  double dot_product = 0.0;
  for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
  {
    dot_product += first[cur_coord] * second[cur_coord];
  }
  return dot_product;
}



double vector_angle(const std::vector<double> &first,
                    const std::vector<double> &second)
{
  double length = vector_length(first) * vector_length(second);
  double dot_product = vector_dot_product(first, second);
  double cosine = dot_product / length;
  if(std::abs(cosine) >= 1.0)
  {
    if(dot_product >= 0.0)
    {
      return 0.0;
    }
    else
    {
      return M_PI;
    }
  }
  return std::acos(cosine);
}



std::vector<double> vector_2d_divide(const std::vector<double> &vec,
                                     double num)
{
  std::vector<double> ret(axes_2d_num, 0.0);
  for(std::size_t cur_coord = 0; cur_coord < axes_2d_num; ++cur_coord)
  {
    ret[cur_coord] = vec[cur_coord] / num;
  }
  return ret;
}



double vector_2d_length(const std::vector<double> &vec)
{
  return std::hypot(vec[0], vec[1]);
}



unsigned long long int calc_norms::normal_to_key(
  const std::vector<double> &norm)
{
  unsigned long long int key = 0;
  for(std::size_t cur_norm = 0; cur_norm < axes_num; ++cur_norm)
  {
    key +=
      static_cast<unsigned long long int>(
        to_integer_multiply +
        std::round(norm[cur_norm] * to_integer_multiply)) <<
      to_key_shift[cur_norm];
  }
  return key;
}

std::vector<double> calc_norms::key_to_normal(unsigned long long int key)
{
  std::vector<double> norm(axes_num, 0.0);
  for(std::size_t cur_norm = 0; cur_norm < axes_num; ++cur_norm)
  {
    unsigned long long int natural_norm =
      (key >> to_key_shift[cur_norm]) & key_to_normal_mask;
    norm[cur_norm] = (natural_norm / to_integer_multiply) - vector_scale_val;
  }
  return norm;
}



// ============================================================================
// Data structures.
// ============================================================================



model_extreme_points::model_extreme_points()
: extreme_points_pair(
    std::vector<double>(axes_num, -std::numeric_limits<double>::max()),
    std::vector<double>(axes_num,  std::numeric_limits<double>::max())
  )
{
}

model_extreme_points::model_extreme_points(
  const std::vector<double> &max,
  const std::vector<double> &min)
: extreme_points_pair(max, min)
{
}

model_extreme_points::model_extreme_points(
  std::vector<double> &&max,
  std::vector<double> &&min)
: extreme_points_pair(max, min)
{
}



std::vector<double> &model_extreme_points::max()
{
  return extreme_points_pair.first;
}

const std::vector<double> &model_extreme_points::max() const
{
  return extreme_points_pair.first;
}

std::vector<double> &model_extreme_points::min()
{
  return extreme_points_pair.second;
}

const std::vector<double> &model_extreme_points::min() const
{
  return extreme_points_pair.second;
}



double model_extreme_points::xmax() const
{
  return max()[0];
}

double model_extreme_points::ymax() const
{
  return max()[1];
}

double model_extreme_points::zmax() const
{
  return max()[2];
}


double model_extreme_points::xmin() const
{
  return min()[0];
}

double model_extreme_points::ymin() const
{
  return min()[1];
}

double model_extreme_points::zmin() const
{
  return min()[2];
}



void model_extreme_points::set_xmax(double new_xmax)
{
  max()[0] = new_xmax;
}

void model_extreme_points::set_ymax(double new_ymax)
{
  max()[1] = new_ymax;
}

void model_extreme_points::set_zmax(double new_zmax)
{
  max()[2] = new_zmax;
}


void model_extreme_points::set_xmin(double new_xmin)
{
  min()[0] = new_xmin;
}

void model_extreme_points::set_ymin(double new_ymin)
{
  min()[1] = new_ymin;
}

void model_extreme_points::set_zmin(double new_zmin)
{
  min()[2] = new_zmin;
}



void model_extreme_points::get_most_extreme_cmp_cur(
  const model_extreme_points &other)
{
  for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
  {
    if(max()[cur_coord] < other.max()[cur_coord])
    {
      max()[cur_coord] = other.max()[cur_coord];
    }
    if(min()[cur_coord] > other.min()[cur_coord])
    {
      min()[cur_coord] = other.min()[cur_coord];
    }
  }
}

void model_extreme_points::get_most_extreme_cmp_cur(
  const std::vector<double> &point)
{
  for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
  {
    if(max()[cur_coord] < point[cur_coord])
    {
      max()[cur_coord] = point[cur_coord];
    }
    if(min()[cur_coord] > point[cur_coord])
    {
      min()[cur_coord] = point[cur_coord];
    }
  }
}

void model_extreme_points::get_most_extreme(
  const std::vector<std::vector<double>> &points)
{
  for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
  {
    auto result =
      std::minmax_element(
        points.begin(), points.end(),
        [&](std::vector<double> const& a, std::vector<double> const& b)
        {
          return a[cur_coord] < b[cur_coord];
        }
      );
    min()[cur_coord] = (*result.first)[cur_coord];
    max()[cur_coord] = (*result.second)[cur_coord];
  }
}



std::vector<double> model_extreme_points::get_center()
{
  // Getting middle point of model as middle of extreme points.
  std::vector<double> box_lengths = vector_minus(max(), min());
  std::vector<double> box_half_lengths = vector_divide(box_lengths, 2);
  std::vector<double> center_point = vector_plus(box_half_lengths, min());
  return center_point;
}



model_offset::model_offset()
: offset_point(axes_num, 0.0)
{
}

model_offset::model_offset(const std::vector<double> &offset_point_arg)
: offset_point(offset_point_arg)
{
}

model_offset::model_offset(std::vector<double> &&offset_point_arg)
: offset_point(offset_point_arg)
{
}

double model_offset::x_off() const
{
  return offset_point[0];
}

double model_offset::y_off() const
{
  return offset_point[1];
}

double model_offset::z_off() const
{
  return offset_point[2];
}

void model_offset::set_x_off(double new_x_off)
{
  offset_point[0] = new_x_off;
}

void model_offset::set_y_off(double new_y_off)
{
  offset_point[1] = new_y_off;
}

void model_offset::set_z_off(double new_z_off)
{
  offset_point[2] = new_z_off;
}



face::face(int numVerts_arg)
: numVerts(numVerts_arg),
  color_id(0),
  wheel_id(invalid::wheel_id),
  weapon_id(invalid::weapon_id),
  norm(std::vector<double>(axes_num, 0.0)),
  w(0.0),
  verts(std::vector<int>(numVerts, invalid::vert_id)),
  vertNorms(std::vector<int>(numVerts, invalid::vertNorm_id))
{
}



polyhedron::polyhedron()
: numVerts(0),
  numVertNorms(0),
  numFaces(0),
  numVertTotal(0),
  numVertsPerPoly(0),
  extreme_points(),
  offset(),
  rmax(0.0),
  volume(0.0),
  rcm(axes_num, 0.0),
  J(axes_num, std::vector<double>(axes_num, 0.0)),
  bodyColorOffset(invalid::bodyColorOffset),
  bodyColorShift(invalid::bodyColorShift),
  ref_vert_one_ind(invalid::ref_vert_ind, invalid::ref_vert_ind),
  ref_vert_one(nullptr),
  ref_vert_two_ind(invalid::ref_vert_ind, invalid::ref_vert_ind),
  ref_vert_two(nullptr),
  ref_angle(0.0),
  wheel_id(invalid::wheel_id),
  volume_overwritten(false),
  rcm_overwritten(false),
  J_overwritten(false)
{
}



polyhedron::polyhedron(int numVerts_arg,
                       int numVertNorms_arg,
                       int numFaces_arg,
                       int verts_per_poly_arg)
: numVerts(numVerts_arg),
  numVertNorms(numVertNorms_arg),
  numFaces(numFaces_arg),
  numVertTotal(numFaces_arg * verts_per_poly_arg),
  numVertsPerPoly(verts_per_poly_arg),
  extreme_points(),
  offset(),
  rmax(0.0),
  volume(0.0),
  rcm(axes_num, 0.0),
  J(axes_num, std::vector<double>(axes_num, 0.0)),
  bodyColorOffset(invalid::bodyColorOffset),
  bodyColorShift(invalid::bodyColorShift),
  verts(
    std::vector<std::vector<double>>(numVerts, std::vector<double>(axes_num))),
  vertNorms(
    std::vector<std::vector<double>>(numVertNorms,
                                     std::vector<double>(axes_num))),
  faces(std::vector<face>(numFaces, face(verts_per_poly_arg))),
  ref_vert_one_ind(invalid::ref_vert_ind, invalid::ref_vert_ind),
  ref_vert_one(nullptr),
  ref_vert_two_ind(invalid::ref_vert_ind, invalid::ref_vert_ind),
  ref_vert_two(nullptr),
  ref_angle(0.0),
  wheel_id(invalid::wheel_id),
  volume_overwritten(false),
  rcm_overwritten(false),
  J_overwritten(false)
{
}





void polyhedron::invertVertNorms()
{
  for(auto &&vertNorm : vertNorms)
  {
    vector_invert_self(vertNorm);
  }
}

void polyhedron::reverse_polygons_orientation()
{
  for(std::size_t cur_poly = 0; cur_poly < numFaces; ++cur_poly)
  {
    std::vector<std::size_t> newVerts(numVertsPerPoly, 0);
    std::vector<std::size_t> newVertNorms(numVertsPerPoly, 0);
    for(std::size_t orig_vert_f_ind = 0, new_vert_f_ind = numVertsPerPoly - 1;
        orig_vert_f_ind < numVertsPerPoly;
        ++orig_vert_f_ind, --new_vert_f_ind)
    {
      newVerts[new_vert_f_ind] =
        faces[cur_poly].verts[orig_vert_f_ind];
      newVertNorms[new_vert_f_ind] =
        faces[cur_poly].vertNorms[orig_vert_f_ind];
    }
    for(std::size_t vert_f_ind = 0; vert_f_ind < numVertsPerPoly; ++vert_f_ind)
    {
      faces[cur_poly].verts[vert_f_ind] = newVerts[vert_f_ind];
      faces[cur_poly].vertNorms[vert_f_ind] = newVertNorms[vert_f_ind];
    }
  }
}



std::vector<double> polyhedron::face_calc_normal(std::size_t face_ind)
{
  face &cur_face = faces[face_ind];
  std::vector<double> normal(axes_num, 0.0);
  for(std::size_t cur_vert = 0; cur_vert < numVertsPerPoly; ++cur_vert)
  {
    const std::vector<double> &current =
      verts[cur_face.verts[cur_vert]];
    const std::vector<double> &next =
      verts[cur_face.verts[(cur_vert + 1) % numVertsPerPoly]];
    for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
    {
      const std::vector<std::size_t> &axes =
        axes_by_plane_continuous[cur_coord];
      normal[cur_coord] += (current[axes[0]] - next[axes[0]]) *
                           (current[axes[1]] + next[axes[1]]);
    }
  }
  return normal;
}



double polyhedron::face_calc_offset_w(std::size_t face_ind)
{
  face &cur_face = faces[face_ind];
  double offset_w = 0.0;
  for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
  {
    offset_w -= cur_face.norm[cur_coord] * verts[cur_face.verts[0]][cur_coord];
  }
  return offset_w;
}



// Must be called again if model was moved.
void polyhedron::faces_calc_params()
{
  std::size_t faces_size = faces.size();
  std::unordered_set<std::size_t> bad_polygons;
  bad_polygons.reserve(faces_size);

  for(std::size_t face_ind = 0; face_ind < faces_size; ++face_ind)
  {
    face &cur_face = faces[face_ind];

    cur_face.norm = face_calc_normal(face_ind);

    vector_scale_self(vector_scale_val, cur_face.norm);

    cur_face.w = face_calc_offset_w(face_ind);

    if(vector_length(cur_face.norm) == 0.0)
    {
      bad_polygons.insert(face_ind);
    }
  }

  // Deleting bad polygons.
  std::size_t cur_f_n = 0;
  faces.erase(
    std::remove_if(
      faces.begin(), faces.end(),
      [&](const face &f)
      {
        return bad_polygons.count(cur_f_n++);
      }
    ),
    faces.end()
  );
  numFaces = faces.size();
  numVertTotal = numFaces * numVertsPerPoly;
}



void polyhedron::faces_calc_params_inv_neg_vol()
{
  // If volume is negative, vertices have wrong order.
  // They must be reversed again in that case.
  faces_calc_params(); // Must be called before "check_volume()".
  if(check_volume() < 0)
  {
    invertVertNorms();
    reverse_polygons_orientation();
    faces_calc_params();
  }
}



double polyhedron::get_vertex_angle(std::size_t face_ind, std::size_t vert_ind)
{
  face &cur_face = faces[face_ind];
  std::size_t prev_ind =
    (vert_ind + cur_face.numVerts - 1) % cur_face.numVerts;
  std::size_t next_ind = (vert_ind + 1) % cur_face.numVerts;
  std::vector<double> &prev_vertex = verts[cur_face.verts[prev_ind]];
  std::vector<double> &cur_vertex =  verts[cur_face.verts[vert_ind]];
  std::vector<double> &next_vertex = verts[cur_face.verts[next_ind]];
  return vector_angle(vector_minus(cur_vertex, prev_vertex),
                      vector_minus(cur_vertex, next_vertex));
}



// Must be called after faces_calc_params().
void polyhedron::recalc_vertNorms(double max_smooth_angle)
{
  // Creating norm per each vertex per face.
  std::vector<std::vector<double>> raw_vertNorms(
    numVertTotal,
    std::vector<double>(axes_num, 0.0));

  // Assigning unique id to each norm per vertex per face.
  for(std::size_t face_ind = 0, norm_ind = 0; face_ind < numFaces; ++face_ind)
  {
    for(std::size_t norm_f_ind = 0;
        norm_f_ind < numVertsPerPoly;
        ++norm_ind, ++norm_f_ind)
    {
      faces[face_ind].vertNorms[norm_f_ind] = norm_ind;
    }
  }

  // Calculating non-normalized face normals.
  std::vector<std::vector<double>> raw_face_norms(
    numFaces,
    std::vector<double>(axes_num, 0.0));
  for(std::size_t face_ind = 0; face_ind < numFaces; ++face_ind)
  {
    raw_face_norms[face_ind] = face_calc_normal(face_ind);
  }

  // Calculating face vertex angles.
  std::vector<std::vector<double>> vert_angles(
    numFaces,
    std::vector<double>(numVertsPerPoly, 0.0));
  for(std::size_t face_ind = 0; face_ind < numFaces; ++face_ind)
  {
    for(std::size_t vert_f_ind = 0; vert_f_ind < numVertsPerPoly; ++vert_f_ind)
    {
      vert_angles[face_ind][vert_f_ind] =
        get_vertex_angle(face_ind, vert_f_ind);
    }
  }

  // Getting map per each vertex: face index - vertex face index.
  std::vector<std::unordered_map<std::size_t, std::size_t>>
    vert_to_face_ind_vert_f_ind_pair(numVerts);
  for(std::size_t vert_ind = 0; vert_ind < numVerts; ++vert_ind)
  {
    vert_to_face_ind_vert_f_ind_pair[vert_ind].reserve(
      calc_norms::expected_connected_polygons_per_vertex);
  }

  for(std::size_t face_ind = 0; face_ind < numFaces; ++face_ind)
  {
    face &cur_face = faces[face_ind];
    for(std::size_t vert_f_ind = 0; vert_f_ind < numVertsPerPoly; ++vert_f_ind)
    {
      std::size_t vert_ind = cur_face.verts[vert_f_ind];
      vert_to_face_ind_vert_f_ind_pair[vert_ind][face_ind] = vert_f_ind;
    }
  }

  // Calculating angles between faces.
  std::unordered_map<
    std::pair<std::size_t, std::size_t>,
    double,
    boost::hash<std::pair<std::size_t, std::size_t>>> angles_between_faces;
  angles_between_faces.reserve(
    calc_norms::expected_connected_polygons_per_polygon * numFaces);
  for(std::size_t vert_ind = 0; vert_ind < numVerts; ++vert_ind)
  {
    for(auto face_ind_vert_f_ind_pair :
        vert_to_face_ind_vert_f_ind_pair[vert_ind])
    {
      std::size_t face_ind = face_ind_vert_f_ind_pair.first;
      for(auto face_ind_vert_f_ind_pair_to_cmp :
          vert_to_face_ind_vert_f_ind_pair[vert_ind])
      {
        std::size_t face_ind_to_cmp = face_ind_vert_f_ind_pair_to_cmp.first;
        // Ignore self.
        if(face_ind == face_ind_to_cmp)
        {
          continue;
        }

        std::pair<std::size_t, std::size_t> face_inds_pair =
          {face_ind, face_ind_to_cmp};
        if(face_inds_pair.first > face_inds_pair.second)
        {
          std::swap(face_inds_pair.first, face_inds_pair.second);
        }

        // Skip if already in map.
        if(angles_between_faces.count(face_inds_pair))
        {
          continue;
        }
        double angle =
          vector_angle(faces[face_inds_pair.first].norm,
                       faces[face_inds_pair.second].norm);
        angles_between_faces[face_inds_pair] = angle;
      }
    }
  }

  // Using angles between faces to determine
  // whether vertices between faces should be smooth.
  std::unordered_set<
    std::pair<std::size_t, std::size_t>,
    boost::hash<std::pair<std::size_t, std::size_t>>>
      smooth_faces;
  smooth_faces.reserve(angles_between_faces.size());
  for(auto face_pair_to_angle : angles_between_faces)
  {
    if(face_pair_to_angle.second - max_smooth_angle < distinct_distance)
    {
      smooth_faces.insert(face_pair_to_angle.first);
    }
  }



  // https://www.bytehazard.com/articles/vertnorm.html
  // Accumulating normals in each face.
  for(std::size_t face_ind = 0; face_ind < numFaces; ++face_ind)
  {
    face &cur_face = faces[face_ind];
    for(std::size_t vert_f_ind = 0; vert_f_ind < numVertsPerPoly; ++vert_f_ind)
    {
      std::size_t vert_ind = cur_face.verts[vert_f_ind];
      std::size_t norm_ind = cur_face.vertNorms[vert_f_ind];
      std::vector<double> &cur_norm = raw_vertNorms[norm_ind];
      // No need to get weight per surface since non-normalized
      // face norm length is directly proportional to surface area of face.
      cur_norm = vector_multiply(raw_face_norms[face_ind],
                                 vert_angles[face_ind][vert_f_ind]);

      for(auto face_ind_vert_f_ind_pair_to_cmp :
          vert_to_face_ind_vert_f_ind_pair[vert_ind])
      {
        std::size_t face_ind_to_cmp = face_ind_vert_f_ind_pair_to_cmp.first;

        // Ignore self.
        if(face_ind == face_ind_to_cmp)
        {
          continue;
        }

        std::pair<std::size_t, std::size_t> face_inds_pair =
          {face_ind, face_ind_to_cmp};
        if(face_inds_pair.first > face_inds_pair.second)
        {
          std::swap(face_inds_pair.first, face_inds_pair.second);
        }

        // Criteria for hard-edges.
        if(!smooth_faces.count(face_inds_pair))
        {
          continue;
        }

        // Accumulate normal.
        std::size_t vert_f_ind_to_cmp = face_ind_vert_f_ind_pair_to_cmp.second;
        // No need to get weight per surface since non-normalized
        // face norm length is directly proportional to surface area of face.
        std::vector<double> norm_to_add =
          vector_multiply(raw_face_norms[face_ind_to_cmp],
                          vert_angles[face_ind_to_cmp][vert_f_ind_to_cmp]);
        vector_plus_self(cur_norm, norm_to_add);
      }
    }
  }





  // Normalizing vertex normals.
  for(auto &&raw_vertNorm : raw_vertNorms)
  {
    vector_scale_self(vector_scale_val, raw_vertNorm);
  }



  // Reducing number of normals.
  std::size_t raw_vertNorms_size = raw_vertNorms.size();
  std::unordered_map<unsigned long long int, std::size_t>
    normal_val_to_norm_ind;
  normal_val_to_norm_ind.reserve(raw_vertNorms_size);

  vertNorms = std::vector<std::vector<double>>();
  vertNorms.reserve(raw_vertNorms_size);
  std::size_t norm_ind = 0;

  for(auto &&face : faces)
  {
    for(std::size_t norm_f_ind = 0; norm_f_ind < numVertsPerPoly; ++norm_f_ind)
    {
      std::size_t raw_norm_ind = face.vertNorms[norm_f_ind];
      unsigned long long int vert_norm_val_key =
        calc_norms::normal_to_key(raw_vertNorms[raw_norm_ind]);

      if(normal_val_to_norm_ind.count(vert_norm_val_key))
      {
        face.vertNorms[norm_f_ind] = normal_val_to_norm_ind[vert_norm_val_key];
      }
      else
      {
        vertNorms.push_back(calc_norms::key_to_normal(vert_norm_val_key));
        normal_val_to_norm_ind[vert_norm_val_key] = norm_ind;
        face.vertNorms[norm_f_ind] = norm_ind;
        ++norm_ind;
      }
    }
  }

  vertNorms.shrink_to_fit();
  numVertNorms = vertNorms.size();
}



// Must be called after faces_calc_params().
double polyhedron::check_volume()
{
  compVolumeIntegrals(this);
  return T0;
}





void polyhedron::get_extreme_points()
{
  extreme_points.get_most_extreme(verts);
}





std::vector<double> polyhedron::get_model_center()
{
  get_extreme_points();
  return extreme_points.get_center();
}





void polyhedron::move_model_to_point(const std::vector<double> &point_arg)
{
  for(auto &&model_vert : verts)
  {
    vector_plus_self(model_vert, point_arg);
  }
}



void polyhedron::move_model_to_point_inv_neg_vol(
  const std::vector<double> &point_arg)
{
  move_model_to_point(point_arg);
  faces_calc_params_inv_neg_vol();
}



void polyhedron::move_coord_system_to_point(
  const std::vector<double> &point_arg)
{
  for(auto &&model_vert : verts)
  {
    vector_minus_self(model_vert, point_arg);
  }
}



void polyhedron::move_coord_system_to_point_inv_neg_vol(
  const std::vector<double> &point_arg)
{
  move_coord_system_to_point(point_arg);
  faces_calc_params_inv_neg_vol();
}



void polyhedron::move_coord_system_to_center()
{
  std::vector<double> center = get_model_center();
  move_coord_system_to_point(center);
}





void polyhedron::rotate_by_axis(double angle, rotation_axis axis)
{
  if(angle == 0.0)
  {
    return;
  }
  double angle_sin = std::sin(angle);
  double angle_cos = std::cos(angle);
  for(auto &&vert_to_change : verts)
  {
    rotate_point_by_axis(vert_to_change, angle_sin, angle_cos, axis);
  }
  for(auto &&vert_norm_to_change : vertNorms)
  {
    rotate_point_by_axis(vert_norm_to_change, angle_sin, angle_cos, axis);
  }
}



void polyhedron::set_color_id(unsigned int new_color_id,
                              int new_wheel_id,
                              int new_weapon_id)
{
  if(new_color_id != color_ids::invalid_color_id)
  {
    for(auto &&cur_poly : faces)
    {
      // If non-standard color id ended up in *.m3d model,
      // it must be preserved.
      if(cur_poly.color_id < color_ids::max_colors_ids)
      {
        cur_poly.color_id = new_color_id;
      }
    }
  }

  if(new_wheel_id != volInt::invalid::wheel_id)
  {
    for(auto &&cur_poly : faces)
    {
      cur_poly.wheel_id = new_wheel_id;
    }
  }

  if(new_weapon_id != volInt::invalid::weapon_id)
  {
    for(auto &&cur_poly : faces)
    {
      cur_poly.weapon_id = new_weapon_id;
    }
  }
}



// Getting two points which are distant enough from each other by x and z.
bool polyhedron::find_ref_points()
{
  std::size_t n_faces = numFaces;
  std::size_t v_per_poly = faces[0].numVerts;


  for(std::size_t poly_1_ind = 0; poly_1_ind < n_faces; ++poly_1_ind)
  {
    for(std::size_t v_1_ind = 0; v_1_ind < v_per_poly; ++v_1_ind)
    {
      ref_vert_one_ind.first = poly_1_ind;
      ref_vert_one_ind.second = v_1_ind;
      ref_vert_one =
        &verts.at(
          faces[ref_vert_one_ind.first].verts[ref_vert_one_ind.second]);

      // Finding second vert which is far enough from first one by x or z.
      // It's needed to find rotation angle around y axis
      // to get rotation of weapon.
      for(std::size_t poly_2_ind = 0; poly_2_ind < n_faces; ++poly_2_ind)
      {
        for(std::size_t v_2_ind = 0; v_2_ind < v_per_poly; ++v_2_ind)
        {
          const std::vector<double> *cur_point =
            &verts.at(faces[poly_2_ind].verts[v_2_ind]);

          std::vector<double> rel_to_one =
            vector_minus((*cur_point), (*ref_vert_one));

          if((std::abs(rel_to_one[0]) > distinct_distance ||
                std::abs(rel_to_one[2]) > distinct_distance) &&
             std::abs(rel_to_one[1]) > distinct_distance)
          {
            ref_vert_two_ind.first =  poly_2_ind;
            ref_vert_two_ind.second = v_2_ind;
            ref_vert_two = cur_point;

            // Finding third vert
            // which is not collinear to vert one and vert two.
            // Needed to check whether the model was rotated by other axes.
            for(std::size_t poly_3_ind = 0; poly_3_ind < n_faces; ++poly_3_ind)
            {
              for(std::size_t v_3_ind = 0; v_3_ind < v_per_poly; ++v_3_ind)
              {
                const std::vector<double> *cur_point =
                  &verts.at(faces[poly_3_ind].verts[v_3_ind]);

                double x1 = (*ref_vert_two)[0] - (*ref_vert_one)[0];
                double x2 = (*cur_point)[0] -    (*ref_vert_one)[0];
                double y1 = (*ref_vert_two)[1] - (*ref_vert_one)[1];
                double y2 = (*cur_point)[1] -    (*ref_vert_one)[1];
                double z1 = (*ref_vert_two)[2] - (*ref_vert_one)[2];
                double z2 = (*cur_point)[2] -    (*ref_vert_one)[2];

                // If current vert is not collinear with vert one and vert two.
                if(!(std::abs(x1 * y2 - x2 * y1) < sqr_distinct_distance &&
                     std::abs(x1 * z2 - x2 * z1) < sqr_distinct_distance &&
                     std::abs(y1 * z2 - y2 * z1) < sqr_distinct_distance))
                {
                  ref_vert_three_ind.first = poly_3_ind;
                  ref_vert_three_ind.second = v_3_ind;
                  ref_vert_three = cur_point;

                  goto find_ref_points_end_search_loop;
                }
              }
            }
          }
        }
      }
    }
  }
  find_ref_points_end_search_loop:

  if(ref_vert_one_ind.first < 0 || ref_vert_two_ind.first < 0)
  {
    return false;
  }

  ref_vert_two_rel_to_one =   vector_minus((*ref_vert_two),   (*ref_vert_one));
  ref_vert_three_rel_to_one = vector_minus((*ref_vert_three), (*ref_vert_one));


  ref_angle = std::atan2((*ref_vert_two)[0] - (*ref_vert_one)[0],
                         (*ref_vert_two)[2] - (*ref_vert_one)[2]);
  return true;
}





void polyhedron::calculate_rmax()
{
  rmax = 0.0;

  for(const auto &vert : verts)
  {
    double cur_vert_length = vector_length(vert);
    if(rmax < cur_vert_length)
    {
      rmax = cur_vert_length;
    }
  }
}



void polyhedron::calculate_c3d_properties()
{
  get_extreme_points();

  calculate_rmax();

  compVolumeIntegrals(this);

  if(T0 < 0)
  {
    std::string err_msg;
    if(wheel_id >= 0)
    {
      err_msg.append("Wheel " + std::to_string(wheel_id + 1) +
                     " of input model " + wavefront_obj_path);
    }
    else
    {
      err_msg.append("Input model " + wavefront_obj_path);
    }
    err_msg.append(" has negative volume: \"" + std::to_string(T0) + "\" " +
                   "(before scaling to the in-game units of measurement).");
    throw exception::negative_volume(err_msg);
  }
  // If volume is 0 for main model of *.m3d or first model of *.a3d,
  // the game will freeze while loading models.
  if(T0 == 0.0 && wheel_id < 0)
  {
    throw exception::zero_volume("Input model " + wavefront_obj_path +
                                 " has zero volume.");
  }

  if(!volume_overwritten)
  {
    volume = T0;
  }
  double mass = density * volume;

  if(!rcm_overwritten)
  {
    // Compute center of mass.
    for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
    {
      rcm[cur_coord] = T1[cur_coord] / volume;
    }
  }

  if(!J_overwritten)
  {
    // Compute inertia tensor.
    for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
    {
      const std::vector<std::size_t> &axes =
        axes_by_plane_continuous[cur_coord];
      J[cur_coord][cur_coord] = density * (T2[axes[0]] + T2[axes[1]]);
      J[axes[0]][axes[1]] = J[axes[1]][axes[0]] = -density * TP[axes[0]];
    }
    // Translate inertia tensor to center of mass.
    for(std::size_t cur_coord = 0; cur_coord < axes_num; ++cur_coord)
    {
      const std::vector<std::size_t> &axes =
        axes_by_plane_continuous[cur_coord];
      J[cur_coord][cur_coord] -=
        mass * (rcm[axes[0]] * rcm[axes[0]] + rcm[axes[1]] * rcm[axes[1]]);
      J[axes[0]][axes[1]] = J[axes[1]][axes[0]] +=
        mass * rcm[axes[0]] * rcm[axes[1]];
    }
  }
}





// Extreme points are generated in such order
// they can be used as polygon vertices to find area.
// axis 1
// 3   2
//       axis 0
// 0   1
std::vector<std::vector<std::vector<double>>>
  polyhedron::get_planes_4_extreme_points() const
{
  std::vector<std::vector<std::vector<double>>> plane_4_extreme_points_by_axis(
    axes_num,
    std::vector<std::vector<double>>(generate_bound::plane_extrs_num,
                                     std::vector<double>(axes_2d_num, 0.0)));
  for(std::size_t cur_axis = 0; cur_axis < axes_num; ++cur_axis)
  {
    std::vector<std::size_t> plane_axes = axes_by_plane[cur_axis];

    std::vector<double> plane_max_point(axes_2d_num, 0.0);
    plane_max_point[0] = max_point()[plane_axes[0]];
    plane_max_point[1] = max_point()[plane_axes[1]];

    std::vector<double> plane_min_point(axes_2d_num, 0.0);
    plane_min_point[0] = min_point()[plane_axes[0]];
    plane_min_point[1] = min_point()[plane_axes[1]];

    std::vector<std::vector<double>> plane_minmax_points(
      2,
      std::vector<double>(axes_2d_num, 0.0));
    plane_minmax_points[0][0] = max_point()[plane_axes[0]];
    plane_minmax_points[0][1] = min_point()[plane_axes[1]];

    plane_minmax_points[1][0] = min_point()[plane_axes[0]];
    plane_minmax_points[1][1] = max_point()[plane_axes[1]];

    plane_4_extreme_points_by_axis[cur_axis][0] = plane_min_point;
    plane_4_extreme_points_by_axis[cur_axis][1] = plane_minmax_points[0];
    plane_4_extreme_points_by_axis[cur_axis][2] = plane_max_point;
    plane_4_extreme_points_by_axis[cur_axis][3] = plane_minmax_points[1];
  }
  return plane_4_extreme_points_by_axis;
}



std::vector<std::unordered_map<std::size_t, double>>
  polyhedron::get_verts_plane_lengths_rel_points(
    std::size_t perpendicular_axis,
    const std::vector<std::vector<double>> &verts_arg,
    const std::vector<std::vector<double>> &points_2d,
    const generate_bound::layer_vert_inds &vert_inds)
{
  std::size_t points_2d_size = points_2d.size();
  std::vector<std::unordered_map<std::size_t, double>> plane_lengths(
    points_2d_size);

  for(std::size_t point_2d_ind = 0;
      point_2d_ind < points_2d_size;
      ++point_2d_ind)
  {
    const std::vector<double> &cur_point_2d_value = points_2d[point_2d_ind];
    std::for_each(std::begin(vert_inds), std::end(vert_inds),
      [&](std::size_t vert_ind)
      {
        std::vector<double> vert_rel_to_point_2d(axes_2d_num, 0.0);
        for(std::size_t plane_2d_axis = 0;
            plane_2d_axis < axes_2d_num;
            ++plane_2d_axis)
        {
          std::size_t model_3d_axis =
            axes_by_plane[perpendicular_axis][plane_2d_axis];
          vert_rel_to_point_2d[plane_2d_axis] =
            verts_arg[vert_ind][model_3d_axis] -
            cur_point_2d_value[plane_2d_axis];
        }
        plane_lengths[point_2d_ind][vert_ind] =
          vector_2d_length(vert_rel_to_point_2d);
      });
  }
  return plane_lengths;
}



generate_bound::layer_vert_inds polyhedron::get_min_length_layer_points(
  const std::vector<std::unordered_map<std::size_t, double>>
    &verts_plane_lengths_rel_points)
{
  std::size_t points_size = verts_plane_lengths_rel_points.size();
  generate_bound::layer_vert_inds layer_verts_by_length(points_size);
  for(std::size_t cur_point = 0; cur_point < points_size; ++cur_point)
  {
    const std::unordered_map<std::size_t, double> &cur_v_plane_lengths =
      verts_plane_lengths_rel_points[cur_point];

    auto extreme_vert_ind_itr =
      std::min_element(
        cur_v_plane_lengths.begin(), cur_v_plane_lengths.end(),
        [](const std::pair<std::size_t, double> &a,
           const std::pair<std::size_t, double> &b)
          {
            return a.second < b.second;
          });
    layer_verts_by_length[cur_point] = extreme_vert_ind_itr->first;
  }
  return layer_verts_by_length;
}



double polyhedron::get_plane_area_from_points(
    std::size_t perpendicular_axis,
    const std::vector<std::vector<double>> &verts_arg,
    const generate_bound::layer_vert_inds &vert_inds)
{
  double area = 0.0;
  std::vector<std::size_t> model_3d_axes = axes_by_plane[perpendicular_axis];
  for(std::size_t vert_ind_ind = 0, vert_inds_size = vert_inds.size();
      vert_ind_ind < vert_inds_size;
      ++vert_ind_ind)
  {
    std::size_t cur_vert_ind =  vert_inds[vert_ind_ind];
    std::size_t next_vert_ind = vert_inds[(vert_ind_ind + 1) % vert_inds_size];
    const std::vector<double> &current = verts_arg[cur_vert_ind];
    const std::vector<double> &next =    verts_arg[next_vert_ind];
    area += (current[model_3d_axes[0]] + next[model_3d_axes[0]]) *
            (current[model_3d_axes[1]] - next[model_3d_axes[1]]);
  }
  return std::abs(area) / 2;
}



std::vector<std::vector<double>> polyhedron::get_extr_middle_points(
  std::size_t perpendicular_axis,
  const std::vector<std::vector<double>> &verts_arg,
  const generate_bound::layer_vert_inds &layer_extrs)
{
  std::vector<std::vector<double>> extr_middle_points_2d(
    generate_bound::plane_middle_extr_num,
    std::vector<double>(axes_2d_num, 0.0));
  for(std::size_t cur_middle_point = 0;
      cur_middle_point < generate_bound::plane_middle_extr_num;
      ++cur_middle_point)
  {
    std::vector<double> extr_points_2d_sum(axes_2d_num, 0.0);
    for(auto extr_ind : generate_bound::extr_lines[cur_middle_point])
    {
      const std::vector<double> &extr_point = verts_arg[layer_extrs[extr_ind]];
      for(std::size_t plane_2d_axis = 0;
          plane_2d_axis < axes_2d_num;
          ++plane_2d_axis)
      {
        std::size_t model_3d_axis =
          axes_by_plane[perpendicular_axis][plane_2d_axis];
        extr_points_2d_sum[plane_2d_axis] += extr_point[model_3d_axis];
      }
    }
    extr_middle_points_2d[cur_middle_point] =
      vector_2d_divide(extr_points_2d_sum,
                       generate_bound::extr_lines[cur_middle_point].size());
  }
  return extr_middle_points_2d;
}



// It is assumed that indices of layer are supplied in this order.
// e - extreme point. m - middle point.
//    y
// e3 m2 e2
// m3 m4 m1 x
// e0 m0 e1
polyhedron polyhedron::extr_inds_to_bound(
  const std::vector<std::vector<double>> &verts_arg,
  const generate_bound::layers_inds_of_axis &extr_inds,
  const generate_bound::layers_inds_of_axis &middle_inds,
  generate_bound::model_type type,
  const model_extreme_points *wheel_params_extremes) const
{
  polyhedron bound_model;
  bound_model.verts =
    std::vector<std::vector<double>>(generate_bound::model::num_verts);

  std::size_t end_z_layers_num = extr_inds.size();
  std::size_t extr_size = generate_bound::model::extr_to_end.size();
  std::size_t mid_size = generate_bound::model::middle_to_end.size();

  // Setting vertices from middle and extreme vertices of main model.
  for(std::size_t layer_ind = 0; layer_ind < end_z_layers_num; ++layer_ind)
  {
    std::size_t starting_vert_ind =
      layer_ind * generate_bound::model::num_verts_per_z_layer;
    for(std::size_t cur_extr = 0; cur_extr < extr_size; ++cur_extr)
    {
      std::size_t end_vert_ind =
        generate_bound::model::extr_to_end[cur_extr] + starting_vert_ind;
      bound_model.verts[end_vert_ind] =
        verts_arg[extr_inds.at(layer_ind).at(cur_extr)];
    }
    for(std::size_t cur_mid = 0; cur_mid < mid_size; ++cur_mid)
    {
      std::size_t end_vert_ind =
        generate_bound::model::middle_to_end[cur_mid] + starting_vert_ind;
      bound_model.verts[end_vert_ind] =
        verts_arg[middle_inds.at(layer_ind).at(cur_mid)];
    }
  }
  if(type == generate_bound::model_type::mechos)
  {
    std::size_t start_of_mid_layer =
      generate_bound::model::num_verts_per_z_layer;
    std::size_t start_of_low_layer =
      2 * generate_bound::model::num_verts_per_z_layer;
    std::size_t mid_vert_ind = start_of_mid_layer;
    std::size_t low_vert_ind = start_of_low_layer;
    for(; mid_vert_ind < start_of_low_layer; ++mid_vert_ind, ++low_vert_ind)
    {
      bound_model.verts[low_vert_ind] = bound_model.verts[mid_vert_ind];
      bound_model.verts[low_vert_ind][VOLINT_Z] = zmin();
    }

    // Setting lowest vertices to be at least
    // as high or as low by x and y axes as wheel_params_extremes.
    if(wheel_params_extremes)
    {
      for(std::size_t cur_axis = 0;
          cur_axis <
            generate_bound::model::min_verts_to_adjust_by_wheel.size();
          ++cur_axis)
      {
        double min_value = wheel_params_extremes->min()[cur_axis];
        double max_value = wheel_params_extremes->max()[cur_axis];
        for(auto vert_ind :
            generate_bound::model::min_verts_to_adjust_by_wheel[cur_axis])
        {
          double &vert_coord = bound_model.verts[vert_ind][cur_axis];
          if(vert_coord > min_value)
          {
            vert_coord = min_value;
          }
        }
        for(auto vert_ind :
            generate_bound::model::max_verts_to_adjust_by_wheel[cur_axis])
        {
          double &vert_coord = bound_model.verts[vert_ind][cur_axis];
          if(vert_coord < max_value)
          {
            vert_coord = max_value;
          }
        }
      }
    }

    for(auto &vert_to_center_by_extremes :
          generate_bound::model::min_layer_verts_to_center_by_extremes)
    {
      std::size_t vert_to_center_ind = vert_to_center_by_extremes.first;
      const std::vector<std::size_t> &extremes =
        vert_to_center_by_extremes.second;

      std::vector<double> extr_center(axes_num, 0.0);
      for(auto extr_ind : extremes)
      {
        vector_plus_self(extr_center, bound_model.verts[extr_ind]);
      }
      vector_divide_self(extr_center, extremes.size());

      bound_model.verts[vert_to_center_ind] = extr_center;
    }
  }


  // Creating dummy normal.
  bound_model.vertNorms.push_back({0.0, 0.0, 0.0});

  // Creating faces.
  bound_model.faces = std::vector<face>(generate_bound::model::num_faces,
                                        generate_bound::model::verts_per_poly);
  std::size_t faces_size = generate_bound::model::num_faces;

  // Setting default color_id.
  for(std::size_t face_ind = 0; face_ind < faces_size; ++face_ind)
  {
    bound_model.faces[face_ind].color_id = color_ids::body;
  }
  if(type == generate_bound::model_type::mechos)
  {
    for(auto face_ind : generate_bound::model::zero_reserved_face_inds)
    {
      bound_model.faces[face_ind].color_id = color_ids::zero_reserved;
    }
  }


  // Setting normal indices for faces.
  for(std::size_t face_ind = 0; face_ind < faces_size; ++face_ind)
  {
    face &cur_face = bound_model.faces[face_ind];
    cur_face.vertNorms = std::vector<int>(cur_face.numVerts, 0);
  }


  // Setting vert indices for faces.
  for(std::size_t face_ind = 0; face_ind < faces_size; ++face_ind)
  {
    bound_model.faces[face_ind].verts =
      generate_bound::model::face_ind_to_vert_inds[face_ind];
  }


  bound_model.numVerts = bound_model.verts.size();
  bound_model.numVertNorms = bound_model.vertNorms.size();
  bound_model.numFaces = bound_model.faces.size();
  bound_model.numVertsPerPoly = bound_model.faces[0].numVerts;
  bound_model.numVertTotal =
    bound_model.numFaces * bound_model.numVertsPerPoly;

  bound_model.faces_calc_params();

  return bound_model;
}



polyhedron polyhedron::generate_bound_model(
  const generate_bound::model_type type,
  const std::size_t layers_num,
  const double area_threshold_multiplier,
  const model_extreme_points *wheel_params_extremes) const
{
  std::vector<std::size_t> layers_inds(layers_num);
  std::iota(layers_inds.begin(), layers_inds.end(), 0);



  std::vector<double> model_dimensions(axes_num, 0.0);
  model_dimensions = vector_minus(max_point(), min_point());

  // Getting distance between layers for each axis.
  std::vector<double> layer_step_per_axis(axes_num, 0.0);
  for(std::size_t cur_axis = 0; cur_axis < axes_num; ++cur_axis)
  {
    layer_step_per_axis[cur_axis] =
      model_dimensions[cur_axis] / (layers_num - 1);
  }



  // Getting edges.
  std::unordered_set<
    std::pair<std::size_t, std::size_t>,
    boost::hash<std::pair<std::size_t, std::size_t>>>
      edges;
  edges.reserve(numVertTotal);
  for(std::size_t face_ind = 0; face_ind < numFaces; ++face_ind)
  {
    for(std::size_t vert_f_ind = 0; vert_f_ind < numVertsPerPoly; ++vert_f_ind)
    {
      std::size_t next_vert_f_ind = (vert_f_ind + 1) % numVertsPerPoly;

      std::size_t vert_ind = faces[face_ind].verts[vert_f_ind];
      std::size_t next_vert_ind = faces[face_ind].verts[next_vert_f_ind];

      if(vert_ind > next_vert_ind)
      {
        std::swap(vert_ind, next_vert_ind);
      }
      edges.insert({vert_ind, next_vert_ind});
    }
  }



  // Creating layer_verts from points of intersection between edges and layers.
  std::vector<std::vector<double>> verts_rel_min = verts;
  for(auto &&vert : verts_rel_min)
  {
    vector_minus_self(vert, min_point());
  }

  std::vector<std::vector<double>> layers_verts;
  layers_verts.reserve(
    edges.size() * axes_num * generate_bound::expected_inter_verts_per_edge);
  generate_bound::layers_inds_by_axis layers_vert_inds(axes_num);
  std::size_t cur_layer_vert_ind = 0;
  for(std::size_t cur_axis = 0; cur_axis < axes_num; ++cur_axis)
  {
    double layer_step = layer_step_per_axis[cur_axis];
    const std::vector<std::size_t> &plane_axes = axes_by_plane[cur_axis];
    for(auto edge : edges)
    {
      std::vector<double> first_v_rel_min =  verts_rel_min[edge.first];
      std::vector<double> second_v_rel_min = verts_rel_min[edge.second];

      // If both points are at the same plane
      // which is perpendicular to cur_axis.
      if(std::abs(first_v_rel_min[cur_axis] - second_v_rel_min[cur_axis]) <
         distinct_distance)
      {
        // If this plane is close enough to layer, add those points to layer.
        double middle_cur_axis_coord =
          (first_v_rel_min[cur_axis] + second_v_rel_min[cur_axis]) / 2;
        if(std::abs(std::remainder(middle_cur_axis_coord, layer_step)) <
           distinct_distance)
        {
          std::size_t layer_ind =
            std::round(middle_cur_axis_coord / layer_step);

          layers_verts.push_back(verts[edge.first]);
          layers_vert_inds[cur_axis][layer_ind].push_back(cur_layer_vert_ind);
          ++cur_layer_vert_ind;

          layers_verts.push_back(verts[edge.second]);
          layers_vert_inds[cur_axis][layer_ind].push_back(cur_layer_vert_ind);
          ++cur_layer_vert_ind;
        }
        continue;
      }

      // Making sure that first vert is lower than second vert by cur_axis.
      if(first_v_rel_min[cur_axis] > second_v_rel_min[cur_axis])
      {
        std::swap(first_v_rel_min, second_v_rel_min);
        // Note that "edge" is local std::pair, not reference.
        std::swap(edge.first, edge.second);
      }

      std::size_t low_layer;
      std::size_t top_layer;

      // If lowest point is close to layer,
      // this layer is the low layer of edge.
      if(std::abs(std::remainder(first_v_rel_min[cur_axis], layer_step)) <
         distinct_distance)
      {
        low_layer = std::round(first_v_rel_min[cur_axis] / layer_step);
      }
      // Else the layer above this point is low layer of edge.
      else
      {
        low_layer = std::ceil(first_v_rel_min[cur_axis] / layer_step);
      }

      // If topmost point is close to layer,
      // this layer is the top layer of edge.
      if(std::abs(std::remainder(second_v_rel_min[cur_axis], layer_step)) <
         distinct_distance)
      {
        top_layer = std::round(second_v_rel_min[cur_axis] / layer_step);
      }
      // Else the layer below this point is top layer of edge.
      else
      {
        top_layer = std::floor(second_v_rel_min[cur_axis] / layer_step);
      }

      std::vector<double> direction =
        vector_minus(second_v_rel_min, first_v_rel_min);

      for(std::size_t layer_ind = low_layer, max_layer = top_layer + 1;
          layer_ind < max_layer;
          ++layer_ind)
      {
        std::vector<double> layer_inter_vert(axes_num);

        layer_inter_vert[cur_axis] = layer_ind * layer_step;

        double step_ind =
          (layer_inter_vert[cur_axis] - first_v_rel_min[cur_axis]) /
          direction[cur_axis];
        for(auto plane_axis : plane_axes)
        {
          double axis_step = direction[plane_axis];
          layer_inter_vert[plane_axis] =
            step_ind * axis_step + first_v_rel_min[plane_axis];
        }

        layers_verts.push_back(vector_plus(layer_inter_vert, min_point()));
        layers_vert_inds[cur_axis][layer_ind].push_back(cur_layer_vert_ind);
        ++cur_layer_vert_ind;
      }
    }
  }



  // Getting extreme points per plane.
  // Extreme points are generated in such order
  // they can be used as polygon vertices to find area.
  std::vector<std::vector<std::vector<double>>>
    plane_4_extreme_points_by_axis =
      get_planes_4_extreme_points();



  // Getting plane lengths of vertices relative to each plane extreme point.
  std::vector<
    std::vector<std::vector<std::unordered_map<std::size_t, double>>>>
      layers_lengths_rel_extrs(
        axes_num,
        std::vector<std::vector<std::unordered_map<std::size_t, double>>>(
          layers_num));
  for(std::size_t cur_axis = 0; cur_axis < axes_num; ++cur_axis)
  {
    for(std::size_t layer_ind = 0; layer_ind < layers_num; ++layer_ind)
    {
      layers_lengths_rel_extrs[cur_axis][layer_ind] =
        get_verts_plane_lengths_rel_points(
          cur_axis,
          layers_verts,
          plane_4_extreme_points_by_axis[cur_axis],
          layers_vert_inds[cur_axis][layer_ind]);
    }
  }



  // Getting extreme vertices for each layer.
  // Extreme vertices are generated in such order
  // they can be used as polygon vertices to find area.
  // 3 2
  // 0 1
  generate_bound::layers_inds_by_axis layers_inds_extreme_points(axes_num);
  for(std::size_t cur_axis = 0; cur_axis < axes_num; ++cur_axis)
  {
    for(std::size_t layer_ind = 0; layer_ind < layers_num; ++layer_ind)
    {
      const generate_bound::layer_vert_inds &cur_layer_vert_inds =
        layers_vert_inds[cur_axis][layer_ind];
      if(cur_layer_vert_inds.size() > 0)
      {
        layers_inds_extreme_points[cur_axis][layer_ind] =
          get_min_length_layer_points(
            layers_lengths_rel_extrs[cur_axis][layer_ind]);
      }
    }
  }



  // Getting area per layer.
  std::vector<std::vector<double>> layers_areas(
    axes_num, std::vector<double>(layers_num, 0.0));
  for(std::size_t cur_axis = 0; cur_axis < axes_num; ++cur_axis)
  {
    for(std::size_t layer_ind = 0; layer_ind < layers_num; ++layer_ind)
    {
      layers_areas[cur_axis][layer_ind] =
        get_plane_area_from_points(
          cur_axis,
          layers_verts,
          layers_inds_extreme_points[cur_axis][layer_ind]);
    }
  }



  // Finding topmost layer with most area.
  std::vector<std::size_t> layer_most_areas_inds(axes_num);
  std::vector<double> layer_most_areas(axes_num, 0.0);
  for(std::size_t cur_axis = 0; cur_axis < axes_num; ++cur_axis)
  {
    std::vector<double> &layer_areas = layers_areas[cur_axis];
    auto layer_ind_itr =
      std::max_element(
        layers_inds.begin(), layers_inds.end(),
        [&](std::size_t a, std::size_t b)
          {
            if(std::abs(layer_areas[a] - layer_areas[b]) < distinct_distance)
            {
              return a < b;
            }
            return layer_areas[a] < layer_areas[b];
          });
    layer_most_areas_inds[cur_axis] = (*layer_ind_itr);
    layer_most_areas[cur_axis] = layer_areas[(*layer_ind_itr)];
  }



  // Getting area treshold to find top and bottom layers.
  std::vector<double> area_thresholds(axes_num, 0.0);
  for(std::size_t cur_axis = 0; cur_axis < axes_num; ++cur_axis)
  {
    area_thresholds[cur_axis] =
      layer_most_areas[cur_axis] * area_threshold_multiplier;
  }



  // Finding max and min layers.
  std::vector<std::size_t> max_layers(axes_num);
  std::vector<std::size_t> min_layers(axes_num);
  for(std::size_t cur_axis = 0; cur_axis < axes_num; ++cur_axis)
  {
    std::vector<double> &layer_areas = layers_areas[cur_axis];
    double area_threshold = area_thresholds[cur_axis];
    std::size_t layer_ind;

    // Max.
    layer_ind = layer_areas.size() - 1;
    for(auto layer_area_ritr = layer_areas.rbegin();
        layer_area_ritr != layer_areas.rend();
        ++layer_area_ritr, --layer_ind)
    {
      if((*layer_area_ritr) >= area_threshold)
      {
        max_layers[cur_axis] = layer_ind;
        break;
      }
    }
    // Min.
    layer_ind = 0;
    for(auto layer_area_itr = layer_areas.begin();
        layer_area_itr != layer_areas.end();
        ++layer_area_itr, ++layer_ind)
    {
      if((*layer_area_itr) >= area_threshold)
      {
        min_layers[cur_axis] = layer_ind;
        break;
      }
    }
  }
  if(type == generate_bound::model_type::mechos)
  {
    min_layers[VOLINT_Z] = 0;
  }



  // Getting layer-defined bounds from max and min layers.
  model_extreme_points layer_bounds;
  for(std::size_t cur_axis = 0; cur_axis < axes_num; ++cur_axis)
  {
    layer_bounds.max()[cur_axis] =
      ((max_layers[cur_axis]) * layer_step_per_axis[cur_axis]) +
      min_point()[cur_axis];
    layer_bounds.min()[cur_axis] =
      min_layers[cur_axis] * layer_step_per_axis[cur_axis] +
      min_point()[cur_axis];
  }



  // Adjusting layer verts so they will be within layer_bounds.
  for(std::size_t cur_axis = 0; cur_axis < axes_num; ++cur_axis)
  {
    for(auto &&layer_vert : layers_verts)
    {
      if(layer_vert[cur_axis] > layer_bounds.max()[cur_axis])
      {
        layer_vert[cur_axis] = layer_bounds.max()[cur_axis];
      }
      else if(layer_vert[cur_axis] < layer_bounds.min()[cur_axis])
      {
        layer_vert[cur_axis] = layer_bounds.min()[cur_axis];
      }
    }
  }



  // Adding indices of higher verts to topmost layer and
  // indices of lower verts to lowest layer.
  for(std::size_t cur_axis = 0; cur_axis < axes_num; ++cur_axis)
  {
    generate_bound::layer_vert_inds &top_layer_vert_inds =
      layers_vert_inds[cur_axis][max_layers[cur_axis]];
    generate_bound::layer_vert_inds &low_layer_vert_inds =
      layers_vert_inds[cur_axis][min_layers[cur_axis]];

    for(std::size_t cur_layer = (layers_num - 1);
        cur_layer > max_layers[cur_axis];
        --cur_layer)
    {
      generate_bound::layer_vert_inds &higher_vert_inds =
        layers_vert_inds[cur_axis][cur_layer];
      top_layer_vert_inds.insert(top_layer_vert_inds.end(),
        higher_vert_inds.begin(), higher_vert_inds.end());
    }

    for(std::size_t cur_layer = 0;
        cur_layer < min_layers[cur_axis];
        ++cur_layer)
    {
      generate_bound::layer_vert_inds &lower_vert_inds =
        layers_vert_inds[cur_axis][cur_layer];
      low_layer_vert_inds.insert(low_layer_vert_inds.end(),
        lower_vert_inds.begin(), lower_vert_inds.end());
    }
  }



  std::size_t end_layers_num;
  if(type == generate_bound::model_type::mechos)
  {
    end_layers_num = generate_bound::end_z_layers_num_mechos;
  }
  else if(type == generate_bound::model_type::other)
  {
    end_layers_num = generate_bound::end_z_layers_num_other;
  }
  std::vector<std::size_t> bound_defining_z_layers =
  {
    max_layers[VOLINT_Z],
    layer_most_areas_inds[VOLINT_Z],
    min_layers[VOLINT_Z],
  };



  // Getting plane lengths of vertices relative to plane extreme point.
  // Finding lengths only for vertices of bound defining z layers.
  std::vector<std::vector<std::unordered_map<std::size_t, double>>>
    z_layers_lengths(end_layers_num);

  for(std::size_t end_layer = 0; end_layer < end_layers_num; ++end_layer)
  {
    z_layers_lengths[end_layer] =
      get_verts_plane_lengths_rel_points(
        VOLINT_Z,
        layers_verts,
        plane_4_extreme_points_by_axis[VOLINT_Z],
        layers_vert_inds[VOLINT_Z][bound_defining_z_layers[end_layer]]);
  }



  // Getting extreme vertices for bound defining z layers.
  // Extreme vertices are generated in such order
  // they can be used as polygon vertices to find area.
  // 3 2
  // 0 1
  generate_bound::layers_inds_of_axis layers_extrs_inds;
  for(std::size_t end_layer = 0; end_layer < end_layers_num; ++end_layer)
  {
    layers_extrs_inds[end_layer] =
      get_min_length_layer_points(z_layers_lengths[end_layer]);
  }



  // layers_extrs_inds will be used to form end bound model.
  // Finding medium points to form
  // standard Vangers bound model with 9 vertices per side.
  std::vector<std::vector<std::vector<double>>> middle_points(end_layers_num);
  for(std::size_t end_layer = 0; end_layer < end_layers_num; ++end_layer)
  {
    middle_points[end_layer] =
      get_extr_middle_points(
        VOLINT_Z,
        layers_verts,
        layers_extrs_inds[end_layer]);
  }



  // Getting lengths of verts relative to middle points.
  std::vector<std::vector<std::unordered_map<std::size_t, double>>>
    verts_lengths_rel_middle_points(end_layers_num);
  for(std::size_t end_layer = 0; end_layer < end_layers_num; ++end_layer)
  {
    verts_lengths_rel_middle_points[end_layer] =
      get_verts_plane_lengths_rel_points(
        VOLINT_Z,
        layers_verts,
        middle_points[end_layer],
        layers_vert_inds[VOLINT_Z][bound_defining_z_layers[end_layer]]);
  }



  // Assigning vert with minimum length as middle point.
  generate_bound::layers_inds_of_axis middle_vert_inds;
  for(std::size_t end_layer = 0; end_layer < end_layers_num; ++end_layer)
  {
    middle_vert_inds[end_layer] =
      get_min_length_layer_points(verts_lengths_rel_middle_points[end_layer]);
  }



  return extr_inds_to_bound(layers_verts,
                            layers_extrs_inds,
                            middle_vert_inds,
                            type,
                            wheel_params_extremes);
}



std::pair<std::vector<double>, std::vector<double>> &
  polyhedron::extreme_points_pair()
{
  return extreme_points.extreme_points_pair;
}

const std::pair<std::vector<double>, std::vector<double>> &
  polyhedron::extreme_points_pair() const
{
  return extreme_points.extreme_points_pair;
}

std::vector<double> &polyhedron::max_point()
{
  return extreme_points.max();
}

const std::vector<double> &polyhedron::max_point() const
{
  return extreme_points.max();
}

std::vector<double> &polyhedron::min_point()
{
  return extreme_points.min();
}

const std::vector<double> &polyhedron::min_point() const
{
  return extreme_points.min();
}


double polyhedron::xmax() const
{
  return extreme_points.xmax();
}

double polyhedron::ymax() const
{
  return extreme_points.ymax();
}

double polyhedron::zmax() const
{
  return extreme_points.zmax();
}


double polyhedron::xmin() const
{
  return extreme_points.xmin();
}

double polyhedron::ymin() const
{
  return extreme_points.ymin();
}

double polyhedron::zmin() const
{
  return extreme_points.zmin();
}


void polyhedron::set_xmax(double new_xmax)
{
  extreme_points.set_xmax(new_xmax);
}

void polyhedron::set_ymax(double new_ymax)
{
  extreme_points.set_ymax(new_ymax);
}

void polyhedron::set_zmax(double new_zmax)
{
  extreme_points.set_zmax(new_zmax);
}


void polyhedron::set_xmin(double new_xmin)
{
  extreme_points.set_xmin(new_xmin);
}

void polyhedron::set_ymin(double new_ymin)
{
  extreme_points.set_ymin(new_ymin);
}

void polyhedron::set_zmin(double new_zmin)
{
  extreme_points.set_zmin(new_zmin);
}



std::vector<double> &polyhedron::offset_point()
{
  return offset.offset_point;
}

const std::vector<double> &polyhedron::offset_point() const
{
  return offset.offset_point;
}


double polyhedron::x_off() const
{
  return offset.x_off();
}

double polyhedron::y_off() const
{
  return offset.y_off();
}

double polyhedron::z_off() const
{
  return offset.z_off();
}


void polyhedron::set_x_off(double new_x_off)
{
  offset.set_x_off(new_x_off);
}

void polyhedron::set_y_off(double new_y_off)
{
  offset.set_y_off(new_y_off);
}

void polyhedron::set_z_off(double new_z_off)
{
  offset.set_z_off(new_z_off);
}



// ============================================================================
// Compute mass properties.
// ============================================================================


// Compute various integrations over projection of face.
void compProjectionIntegrals(POLYHEDRON *p, FACE *f)
{
  double a0, a1, da;
  double b0, b1, db;
  double a0_2, a0_3, a0_4, b0_2, b0_3, b0_4;
  double a1_2, a1_3, b1_2, b1_3;
  double C1, Ca, Caa, Caaa, Cb, Cbb, Cbbb;
  double Cab, Kab, Caab, Kaab, Cabb, Kabb;
  int i;

  P1 = Pa = Pb = Paa = Pab = Pbb = Paaa = Paab = Pabb = Pbbb = 0.0;

  for (i = 0; i < f->numVerts; i++)
  {
    a0 = p->verts[f->verts[i]][A];
    b0 = p->verts[f->verts[i]][B];
    a1 = p->verts[f->verts[(i + 1) % f->numVerts]][A];
    b1 = p->verts[f->verts[(i + 1) % f->numVerts]][B];
    da = a1 - a0;
    db = b1 - b0;
    a0_2 = a0 * a0; a0_3 = a0_2 * a0; a0_4 = a0_3 * a0;
    b0_2 = b0 * b0; b0_3 = b0_2 * b0; b0_4 = b0_3 * b0;
    a1_2 = a1 * a1; a1_3 = a1_2 * a1; 
    b1_2 = b1 * b1; b1_3 = b1_2 * b1;

    C1 =   a1 + a0;
    Ca =   a1 * C1  + a0_2;
    Caa =  a1 * Ca  + a0_3;
    Caaa = a1 * Caa + a0_4;
    Cb =   b1 * (b1 + b0) + b0_2;
    Cbb =  b1 * Cb  + b0_3;
    Cbbb = b1 * Cbb + b0_4;
    Cab =  3 * a1_2 + 2 * a1 * a0 + a0_2;
    Kab =  a1_2 + 2 * a1 * a0 + 3 * a0_2;
    Caab = a0 * Cab + 4 * a1_3;
    Kaab = a1 * Kab + 4 * a0_3;
    Cabb = 4 * b1_3 + 3 * b1_2 * b0 + 2 * b1 * b0_2 + b0_3;
    Kabb = b1_3 + 2 * b1_2 * b0 + 3 * b1 * b0_2 + 4 * b0_3;

    P1 +=   db * C1;
    Pa +=   db * Ca;
    Paa +=  db * Caa;
    Paaa += db * Caaa;
    Pb +=   da * Cb;
    Pbb +=  da * Cbb;
    Pbbb += da * Cbbb;
    Pab +=  db * (b1 * Cab  + b0 * Kab);
    Paab += db * (b1 * Caab + b0 * Kaab);
    Pabb += da * (a1 * Cabb + a0 * Kabb);
  }

  P1 /=    2.0;
  Pa /=    6.0;
  Paa /=   12.0;
  Paaa /=  20.0;
  Pb /=   -6.0;
  Pbb /=  -12.0;
  Pbbb /= -20.0;
  Pab /=   24.0;
  Paab /=  60.0;
  Pabb /= -60.0;
}

void compFaceIntegrals(POLYHEDRON *p, FACE *f)
{
  std::vector<double> n;
  double w;
  double k1, k2, k3, k4;

  compProjectionIntegrals(p, f);

  w = f->w;
  n = f->norm;
  k1 = 1 / n[C]; k2 = k1 * k1; k3 = k2 * k1; k4 = k3 * k1;

  Fa =    k1 * Pa;
  Fb =    k1 * Pb;
  Fc =   -k2 * (n[A] * Pa + n[B] * Pb + w * P1);

  Faa =   k1 * Paa;
  Fbb =   k1 * Pbb;
  Fcc =   k3 * (VOLINT_SQR(n[A]) * Paa +
                2 * n[A] * n[B] * Pab +
                VOLINT_SQR(n[B]) * Pbb +
                w * (2 * (n[A] * Pa + n[B] * Pb) + w * P1));

  Faaa =  k1 * Paaa;
  Fbbb =  k1 * Pbbb;
  Fccc = -k4 * (VOLINT_CUBE(n[A]) * Paaa +
                3 * VOLINT_SQR(n[A]) * n[B] * Paab +
                3 * n[A] * VOLINT_SQR(n[B]) * Pabb +
                VOLINT_CUBE(n[B]) * Pbbb +
                3 * w * (VOLINT_SQR(n[A]) * Paa +
                         2 * n[A] * n[B] * Pab +
                         VOLINT_SQR(n[B]) * Pbb) +
                w * w * (3 * (n[A] * Pa + n[B] * Pb) + w * P1));

  Faab =  k1 * Paab;
  Fbbc = -k2 * (n[A] * Pabb + n[B] * Pbbb + w * Pbb);
  Fcca =  k3 * (VOLINT_SQR(n[A]) * Paaa +
                2 * n[A] * n[B] * Paab +
                VOLINT_SQR(n[B]) * Pabb +
                w * (2 * (n[A] * Paa + n[B] * Pab) + w * Pa));
}

void compVolumeIntegrals(POLYHEDRON *p)
{
  FACE *f;
  double nx, ny, nz;
  int i;

  T0 = T1[VOLINT_X] = T1[VOLINT_Y] = T1[VOLINT_Z] 
     = T2[VOLINT_X] = T2[VOLINT_Y] = T2[VOLINT_Z] 
     = TP[VOLINT_X] = TP[VOLINT_Y] = TP[VOLINT_Z] = 0;

  for (i = 0; i < p->numFaces; i++) {

    f = &p->faces[i];

    nx = fabs(f->norm[VOLINT_X]);
    ny = fabs(f->norm[VOLINT_Y]);
    nz = fabs(f->norm[VOLINT_Z]);
    if (nx > ny && nx > nz) C = VOLINT_X;
    else C = (ny > nz) ? VOLINT_Y : VOLINT_Z;
    A = (C + 1) % 3;
    B = (A + 1) % 3;

    compFaceIntegrals(p, f);

    T0 +=
      f->norm[VOLINT_X] * ((A == VOLINT_X) ? Fa : ((B == VOLINT_X) ? Fb : Fc));

    T1[A] += f->norm[A] * Faa;
    T1[B] += f->norm[B] * Fbb;
    T1[C] += f->norm[C] * Fcc;
    T2[A] += f->norm[A] * Faaa;
    T2[B] += f->norm[B] * Fbbb;
    T2[C] += f->norm[C] * Fccc;
    TP[A] += f->norm[A] * Faab;
    TP[B] += f->norm[B] * Fbbc;
    TP[C] += f->norm[C] * Fcca;
  }

  T1[VOLINT_X] /= 2; T1[VOLINT_Y] /= 2; T1[VOLINT_Z] /= 2;
  T2[VOLINT_X] /= 3; T2[VOLINT_Y] /= 3; T2[VOLINT_Z] /= 3;
  TP[VOLINT_X] /= 2; TP[VOLINT_Y] /= 2; TP[VOLINT_Z] /= 2;
}



} // namespace volInt
