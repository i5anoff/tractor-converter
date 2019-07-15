
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



// Modified for tractor_converter



#include "volInt.hpp"



namespace volInt{



double degrees_to_radians(double degrees)
{
  return degrees * (M_PI / 180.0);
}

double radians_to_degrees(double radians)
{
  return radians * (180.0 / M_PI);
}



/*
// VANGERS SOURCE
// how angle conversions works in vangers code
#define M_PI      3.14159265358979323846

Pi_len = 11
const int  Pi    = 1 << Pi_len;
2048

#define GTOR(x) (double(x)*(M_PI/double(Pi)))
#define RTOG(x) (round(x*(double(Pi)/M_PI)))
*/

double sicher_angle_to_radians(int sicher_angle)
{
  return static_cast<double>(sicher_angle) *
         (M_PI/static_cast<double>(sicher_angle_Pi));
}

int radians_to_sicher_angle(double radians)
{
  return std::round(radians*(static_cast<double>(sicher_angle_Pi)/M_PI));
}



/*
// VANGERS SOURCE
// how rotation works in vangers source
inline Vector Vector::operator* (const DBM& m) const
{
  return
    Vector(
      round(m.a[0]*x + m.a[1]*y + m.a[2]*z),
      round(m.a[3]*x + m.a[4]*y + m.a[5]*z),
      round(m.a[6]*x + m.a[7]*y + m.a[8]*z));
}

inline DBM::DBM(int angle,int axis)
{
//------ Calculate Matrix for ROTATE point an angle ------
  double calpha = Cos(angle);
  double salpha = Sin(angle);
  switch(axis){
    case Z_AXIS:
      a[0]   =  calpha; a[1]   = -salpha; a[2]   = 0;
      a[3]   =  salpha; a[4]   = calpha;  a[5]   = 0;
      a[6]   = 0;    a[7]   = 0;      a[8]   = 1;
      break;
    case X_AXIS:
      a[0]   =  1;    a[1]   =  0;      a[2]   = 0;
      a[3]   =  0;    a[4]   =  calpha; a[5]   = -salpha;
      a[6]   =  0;    a[7]   =  salpha; a[8]   = calpha;
      break;
    case Y_AXIS:
      a[0]   = calpha;  a[1]   =  0;      a[2]   = salpha;
      a[3]   = 0;    a[4]   =  1;      a[5]   = 0;
      a[6]   = -salpha; a[7]   =  0;      a[8]   = calpha;
      break;
    }
}
*/

void rotate_point_by_axis(
  std::vector<double> &point_arg,
  double angle_sin,
  double angle_cos,
  rotation_axis axis)
{
  std::vector<double> point_orig = point_arg;
  if(axis == rotation_axis::x)
  {
    point_arg[1] = angle_cos * point_orig[1] - angle_sin * point_orig[2];
    point_arg[2] = angle_sin * point_orig[1] + angle_cos * point_orig[2];
  }
  if(axis == rotation_axis::y)
  {
    point_arg[0] =  angle_cos * point_orig[0] + angle_sin * point_orig[2];
    point_arg[2] = -angle_sin * point_orig[0] + angle_cos * point_orig[2];
  }
  if(axis == rotation_axis::z)
  {
    point_arg[0] = angle_cos * point_orig[0] - angle_sin * point_orig[1];
    point_arg[1] = angle_sin * point_orig[0] + angle_cos * point_orig[1];
  }
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
  double vec_length =
    std::sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
  double s;
  if(vec_length == 0.0)
  {
    s = 0.0;
  }
  else
  {
    s = norm / vec_length;
  }

  std::vector<double> ret(3, 0.0);
  ret[0] = vec[0] * s;
  ret[1] = vec[1] * s;
  ret[2] = vec[2] * s;

  return ret;
}



void vector_scale_self(double norm, std::vector<double> &vec)
{
  double vec_length =
    std::sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
  double s;
  if(vec_length == 0.0)
  {
    s = 0.0;
  }
  else
  {
    s = norm / vec_length;
  }

  vec[0] *= s;
  vec[1] *= s;
  vec[2] *= s;
}



// not needed
/*
std::vector<double> vector_scale_to_max_coord(double max_coord,
                                              const std::vector<double> &vec)
{
  std::vector<double> ret(3, 0.0);

  double extreme_norm = std::abs(vec[0]);
  double el_1_abs = std::abs(vec[1]);
  if(el_1_abs > extreme_norm)
  {
    extreme_norm = el_1_abs;
  }
  double el_2_abs = std::abs(vec[2]);
  if(el_2_abs > extreme_norm)
  {
    extreme_norm = el_2_abs;
  }
  double s = max_coord / extreme_norm;

  ret[0] = vec[0] * s;
  ret[1] = vec[1] * s;
  ret[2] = vec[2] * s;

  std::cout << "\n\n";
  std::cout << "vector_scale_to_max_coord" << '\n';
  std::cout << "max_coord: " << max_coord << '\n';
  std::cout << "input: " <<
    vec[0] << ", " << vec[1] << ", " << vec[2] << '\n';
  std::cout << "s: " << s << '\n';
  std::cout << "output: " <<
    ret[0] << ", " << ret[1] << ", " << ret[2] << '\n';

  return ret;
}



void vector_scale_self_to_max_coord(double max_coord,
                                    std::vector<double> &vec)
{
  double extreme_norm = std::abs(vec[0]);
  double el_1_abs = std::abs(vec[1]);
  if(el_1_abs > extreme_norm)
  {
    extreme_norm = el_1_abs;
  }
  double el_2_abs = std::abs(vec[2]);
  if(el_2_abs > extreme_norm)
  {
    extreme_norm = el_2_abs;
  }
  double s = max_coord / extreme_norm;

  vec[0] *= s;
  vec[1] *= s;
  vec[2] *= s;
}
*/



void vector_make_zero(std::vector<double> &vec)
{
  vec[0] = 0.0;
  vec[1] = 0.0;
  vec[2] = 0.0;
}



std::vector<double> vector_invert(const std::vector<double> &vec)
{
  std::vector<double> ret(3, 0.0);
  ret[0] = -vec[0];
  ret[1] = -vec[1];
  ret[2] = -vec[2];
  return ret;
}



void vector_invert_self(std::vector<double> &vec)
{
  vec[0] = -vec[0];
  vec[1] = -vec[1];
  vec[2] = -vec[2];
}



std::vector<double> vector_plus(const std::vector<double> &first,
                                const std::vector<double> &second)
{
  std::vector<double> ret(3, 0.0);
  ret[0] = first[0] + second[0];
  ret[1] = first[1] + second[1];
  ret[2] = first[2] + second[2];
  return ret;
}



void vector_plus_self(std::vector<double> &first,
                      const std::vector<double> &second)
{
  first[0] += second[0];
  first[1] += second[1];
  first[2] += second[2];
}



std::vector<double> vector_minus(const std::vector<double> &first,
                                 const std::vector<double> &second)
{
  std::vector<double> ret(3, 0.0);
  ret[0] = first[0] - second[0];
  ret[1] = first[1] - second[1];
  ret[2] = first[2] - second[2];
  return ret;
}



void vector_minus_self(std::vector<double> &first,
                       const std::vector<double> &second)
{
  first[0] -= second[0];
  first[1] -= second[1];
  first[2] -= second[2];
}



std::vector<double> vector_multiply(const std::vector<double> &first,
                                    const std::vector<double> &second)
{
  std::vector<double> ret(3, 0.0);
  ret[0] = first[0] * second[0];
  ret[1] = first[1] * second[1];
  ret[2] = first[2] * second[2];
  return ret;
}



void vector_multiply_self(std::vector<double> &first,
                          const std::vector<double> &second)
{
  first[0] *= second[0];
  first[1] *= second[1];
  first[2] *= second[2];
}



std::vector<double> vector_multiply(const std::vector<double> &vec,
                                    double num)
{
  std::vector<double> ret(3, 0.0);
  ret[0] = vec[0] * num;
  ret[1] = vec[1] * num;
  ret[2] = vec[2] * num;
  return ret;
}



void vector_multiply_self(std::vector<double> &vec,
                          double num)
{
  vec[0] *= num;
  vec[1] *= num;
  vec[2] *= num;
}



std::vector<double> vector_divide(const std::vector<double> &first,
                                  const std::vector<double> &second)
{
  std::vector<double> ret(3, 0.0);
  ret[0] = first[0] / second[0];
  ret[1] = first[1] / second[1];
  ret[2] = first[2] / second[2];
  return ret;
}



void vector_divide_self(std::vector<double> &first,
                        const std::vector<double> &second)
{
  first[0] /= second[0];
  first[1] /= second[1];
  first[2] /= second[2];
}



std::vector<double> vector_divide(const std::vector<double> &vec,
                                  double num)
{
  std::vector<double> ret(3, 0.0);
  ret[0] = vec[0] / num;
  ret[1] = vec[1] / num;
  ret[2] = vec[2] / num;
  return ret;
}



void vector_divide_self(std::vector<double> &vec,
                        double num)
{
  vec[0] /= num;
  vec[1] /= num;
  vec[2] /= num;
}



double vector_length(const std::vector<double> &vec)
{
  return std::sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
}



double vector_length_between(
  const std::vector<double> &first,
  const std::vector<double> &second)
{
  std::vector<double> tmp_vec = vector_minus(first, second);
  return vector_length(tmp_vec);
}



bool vector_equal(
  const std::vector<double> &first,
  const std::vector<double> &second)
{
  if(std::abs(first[0] - second[0]) < distinct_distance &&
     std::abs(first[1] - second[1]) < distinct_distance &&
     std::abs(first[2] - second[2]) < distinct_distance)
  {
    return true;
  }
  return false;
}



double vector_dot_product(const std::vector<double> &first,
                          const std::vector<double> &second)
{
  return first[0] * second[0] + first[1] * second[1] + first[2] * second[2];
}



std::vector<double> vector_cross_product(const std::vector<double> &first,
                                         const std::vector<double> &second)
{
  std::vector<double> ret(3, 0.0);
  ret[0] = first[1] * second[2] - first[2] * second[1];
  ret[1] = first[2] * second[0] - first[0] * second[2];
  ret[2] = first[0] * second[1] - first[1] * second[0];
  return ret;
}



double vector_angle(
  const std::vector<double> &first,
  const std::vector<double> &second)
{
  double length = vector_length(first) * vector_length(second);
  double dot_product = vector_dot_product(first, second);
  double cosine = dot_product/length;
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



void matrix_multiply_self(
  std::vector<std::vector<double>> &mat,
  double num)
{
  vector_multiply_self(mat[0], num);
  vector_multiply_self(mat[1], num);
  vector_multiply_self(mat[2], num);
}



/*
   ============================================================================
   data structures
   ============================================================================
*/



model_extreme_points::model_extreme_points()
: extreme_points_pair(
    std::vector<double>(3, -std::numeric_limits<double>::max()),
    std::vector<double>(3,  std::numeric_limits<double>::max())
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
  for(std::size_t cur_coord = 0; cur_coord < 3; ++cur_coord)
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
  for(std::size_t cur_coord = 0; cur_coord < 3; ++cur_coord)
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
  for(std::size_t cur_coord = 0; cur_coord < 3; ++cur_coord)
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



void model_extreme_points::get_most_extreme(
  const std::vector<const std::vector<double>*> &points)
{
  for(std::size_t cur_coord = 0; cur_coord < 3; ++cur_coord)
  {
    auto result =
      std::minmax_element(
        points.begin(), points.end(),
        [&](const std::vector<double> *a, const std::vector<double> *b)
        {
          return (*a)[cur_coord] < (*b)[cur_coord];
        }
      );
    min()[cur_coord] = (*(*result.first))[cur_coord];
    max()[cur_coord] = (*(*result.second))[cur_coord];
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
: offset_point(3, 0.0)
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



/*
face::face(const face &other)
: numVerts(other.numVerts),
  color_id(other.color_id),
  wheel_weapon_id(other.wheel_weapon_id),
  norm(other.norm),
  w(other.w),
  verts(other.verts),
  vertNorms(other.vertNorms)
{
}
*/


face::face(int numVerts_arg)
: numVerts(numVerts_arg),
  color_id(0),
  wheel_weapon_id(-1),
  norm(std::vector<double>(3, 0.0)),
  w(0.0),
  verts(std::vector<int>(numVerts, -1)),
  vertNorms(std::vector<int>(numVerts, -1))
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
  rcm(3, 0.0),
  J(3, std::vector<double>(3, 0.0)),
  bodyColorOffset(-1),
  bodyColorShift(-1),
  ref_vert_one_ind(-1, -1),
  ref_vert_one(nullptr),
  ref_vert_two_ind(-1, -1),
  ref_vert_two(nullptr),
  ref_angle(0.0),
  wheel_id(-1),
  volume_overwritten(false),
  rcm_overwritten(false),
  J_overwritten(false)
{
}


/*
polyhedron::polyhedron(const polyhedron &other)
: numVerts(other.numVerts),
  numVertNorms(other.numVertNorms),
  numFaces(other.numFaces),
  numVertTotal(other.numVertTotal),
  numVertsPerPoly(other.numVertsPerPoly),
  extreme_points(other.extreme_points),
  offset(other.offset),
  rmax(other.rmax),
  volume(other.volume),
  rcm(other.rcm),
  J(other.J),
  bodyColorOffset(other.bodyColorOffset),
  bodyColorShift(other.bodyColorShift),
  verts(other.verts),
  vertNorms(other.vertNorms),
  faces(other.faces),
  ref_vert_one_ind(other.ref_vert_one_ind),
  ref_vert_one(other.ref_vert_one),
  ref_vert_two_ind(other.ref_vert_two_ind),
  ref_vert_two(other.ref_vert_two),
  ref_angle(other.ref_angle),
  wavefront_obj_path(other.wavefront_obj_path),
  wheels(other.wheels),
  wheels_steer(other.wheels_steer),
  wheels_non_steer(other.wheels_non_steer),
  wheels_ghost(other.wheels_ghost),
  wheels_non_ghost(other.wheels_non_ghost),
  wheel_id(other.wheel_id),
  volume_overwritten(other.volume_overwritten),
  rcm_overwritten(other.rcm_overwritten),
  J_overwritten(other.J_overwritten)
{
}


polyhedron::polyhedron(polyhedron &&other)
: numVerts(other.numVerts),
  numVertNorms(other.numVertNorms),
  numFaces(other.numFaces),
  numVertTotal(other.numVertTotal),
  numVertsPerPoly(other.numVertsPerPoly),
  extreme_points(other.extreme_points),
  offset(other.offset),
  rmax(other.rmax),
  volume(other.volume),
  rcm(other.rcm),
  J(other.J),
  bodyColorOffset(other.bodyColorOffset),
  bodyColorShift(other.bodyColorShift),
  verts(std::move(other.verts)),
  vertNorms(std::move(other.vertNorms)),
  faces(std::move(other.faces)),
  ref_vert_one_ind(other.ref_vert_one_ind),
  ref_vert_one(other.ref_vert_one),
  ref_vert_two_ind(other.ref_vert_two_ind),
  ref_vert_two(other.ref_vert_two),
  ref_angle(other.ref_angle),
  wavefront_obj_path(other.wavefront_obj_path),
  wheels(other.wheels),
  wheels_steer(other.wheels_steer),
  wheels_non_steer(other.wheels_non_steer),
  wheels_ghost(other.wheels_ghost),
  wheels_non_ghost(other.wheels_non_ghost),
  wheel_id(other.wheel_id),
  volume_overwritten(other.volume_overwritten),
  rcm_overwritten(other.rcm_overwritten),
  J_overwritten(other.J_overwritten)
{
}
*/


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
  rcm(3, 0.0),
  J(3, std::vector<double>(3, 0.0)),
  bodyColorOffset(-1),
  bodyColorShift(-1),
  verts(std::vector<std::vector<double>>(numVerts, std::vector<double>(3))),
  vertNorms(
    std::vector<std::vector<double>>(numVertNorms, std::vector<double>(3))),
  faces(std::vector<face>(numFaces, face(verts_per_poly_arg))),
  ref_vert_one_ind(-1, -1),
  ref_vert_one(nullptr),
  ref_vert_two_ind(-1, -1),
  ref_vert_two(nullptr),
  ref_angle(0.0),
  wheel_id(-1),
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
    std::vector<int> newVerts(numVertsPerPoly, 0);
    std::vector<int> newVertNorms(numVertsPerPoly, 0);
    for(int cur_orig_vertex = 0, cur_new_vertex = numVertsPerPoly - 1;
        cur_orig_vertex < numVertsPerPoly;
        ++cur_orig_vertex, --cur_new_vertex)
    {
      newVerts[cur_new_vertex] =
        faces[cur_poly].verts[cur_orig_vertex];
      newVertNorms[cur_new_vertex] =
        faces[cur_poly].vertNorms[cur_orig_vertex];
    }
    for(int cur_vertex = 0;
        cur_vertex < numVertsPerPoly;
        ++cur_vertex)
    {
      faces[cur_poly].verts[cur_vertex] =
        newVerts[cur_vertex];
      faces[cur_poly].vertNorms[cur_vertex] =
        newVertNorms[cur_vertex];
    }
  }
}



std::vector<double> polyhedron::face_calc_normal(std::size_t face_ind)
{
  face &cur_face = faces[face_ind];
  std::vector<double> normal(3, 0.0);
  for(std::size_t cur_vert = 0; cur_vert < numVertsPerPoly; ++cur_vert)
  {
    std::vector<double> current =
      verts[cur_face.verts[cur_vert]];
    std::vector<double> next =
      verts[cur_face.verts[(cur_vert+1)%numVertsPerPoly]];
    normal[VOLINT_X] += (current[VOLINT_Y] - next[VOLINT_Y]) *
                        (current[VOLINT_Z] + next[VOLINT_Z]);
    normal[VOLINT_Y] += (current[VOLINT_Z] - next[VOLINT_Z]) *
                        (current[VOLINT_X] + next[VOLINT_X]);
    normal[VOLINT_Z] += (current[VOLINT_X] - next[VOLINT_X]) *
                        (current[VOLINT_Y] + next[VOLINT_Y]);
  }
  return normal;
}



double polyhedron::face_calc_offset_w(std::size_t face_ind)
{
  face &cur_face = faces[face_ind];
  double offset_w;
  offset_w = - cur_face.norm[VOLINT_X] * verts[cur_face.verts[0]][VOLINT_X]
             - cur_face.norm[VOLINT_Y] * verts[cur_face.verts[0]][VOLINT_Y]
             - cur_face.norm[VOLINT_Z] * verts[cur_face.verts[0]][VOLINT_Z];
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
      [&](const volInt::face &f)
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
  // They must be reversed again in this case.
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
    (vert_ind - 1 + cur_face.numVerts) % cur_face.numVerts;
  std::size_t next_ind =
    (vert_ind + 1) % cur_face.numVerts;
  std::vector<double> &prev_vertex =
    verts[cur_face.verts[prev_ind]];
  std::vector<double> &cur_vertex =
    verts[cur_face.verts[vert_ind]];
  std::vector<double> &next_vertex =
    verts[cur_face.verts[next_ind]];
  return vector_angle(vector_minus(cur_vertex, prev_vertex),
                      vector_minus(cur_vertex, next_vertex));
}



// Must be called after faces_calc_params().
void polyhedron::recalc_vertNorms(double max_smooth_angle)
{
  // Creating norm per each vertex per face.
  std::vector<std::vector<double>> raw_vertNorms(numVertTotal,
                                                 std::vector<double>(3, 0.0));

  // Assigning unique id to each norm per vertex per face.
//  std::cout << "Assigning unique id to each norm per vertex per face." << '\n';
  for(std::size_t face_ind = 0, norm_ind = 0; face_ind < numFaces; ++face_ind)
  {
    for(std::size_t norm_f_ind = 0;
        norm_f_ind < numVertsPerPoly;
        ++norm_ind, ++norm_f_ind)
    {
      faces[face_ind].vertNorms[norm_f_ind] = norm_ind;
//      std::cout << "faces[" << face_ind << "]"
//        ".vertNorms[" << norm_f_ind << "]" <<
//        faces[face_ind].vertNorms[norm_f_ind] << '\n';
    }
  }
//  std::cout << "---------------------------------------------" << '\n';

  // Calculating non-normalized face normals.
//  std::cout << "Calculating non-normalized face normals." << '\n';
  std::vector<std::vector<double>> raw_face_norms(numFaces,
                                                  std::vector<double>(3, 0.0));
  for(std::size_t face_ind = 0; face_ind < numFaces; ++face_ind)
  {
    raw_face_norms[face_ind] = face_calc_normal(face_ind);
//    std::cout << "raw_face_norms[" << face_ind << "]: " <<
//      raw_face_norms[face_ind][0] << " " <<
//      raw_face_norms[face_ind][1] << " " <<
//      raw_face_norms[face_ind][2] << '\n';
  }
//  std::cout << "---------------------------------------------" << '\n';

  // Calculating face vertex angles.
//  std::cout << "Calculating face vertex angles." << '\n';
  std::unordered_map<std::size_t, std::unordered_map<std::size_t, double>>
    vert_angles;
  for(std::size_t face_ind = 0; face_ind < numFaces; ++face_ind)
  {
    for(std::size_t vert_f_ind = 0; vert_f_ind < numVertsPerPoly; ++vert_f_ind)
    {
      vert_angles[face_ind][vert_f_ind] =
        get_vertex_angle(face_ind, vert_f_ind);
//      std::cout << "vert_angles[" << face_ind << "][" << vert_f_ind << "]: " <<
//        vert_angles[face_ind][vert_f_ind] << '\n';
    }
  }
//  std::cout << "---------------------------------------------" << '\n';

  // Getting map: vertex - containing faces.
//  std::cout << "Getting map: vertex - containing faces." << '\n';
  std::unordered_map<std::size_t, std::unordered_set<std::size_t>>
    vert_to_faces_map;
  std::unordered_map<std::size_t, std::unordered_map<std::size_t, std::size_t>>
    vert_to_faces_map_with_vert_f_num;
  for(std::size_t face_ind = 0; face_ind < numFaces; ++face_ind)
  {
    face &cur_face = faces[face_ind];
    for(std::size_t vert_f_ind = 0; vert_f_ind < numVertsPerPoly; ++vert_f_ind)
    {
      std::size_t vert_ind = cur_face.verts[vert_f_ind];
      vert_to_faces_map[vert_ind].insert(face_ind);
      vert_to_faces_map_with_vert_f_num[vert_ind][face_ind] = vert_f_ind;

//      std::cout << "vert_to_faces_map_with_vert_f_num"
//        "[" << vert_ind<< "][" << face_ind << "]: " <<
//        vert_to_faces_map_with_vert_f_num[vert_ind][face_ind] << '\n';
    }
  }
//  std::cout << "---------------------------------------------" << '\n';

  // Calculating angles between faces.
//  std::cout << "Calculating angles between faces." << '\n';
  std::unordered_map<std::size_t, std::unordered_map<std::size_t, double>>
    angles_between_faces;
  for(std::size_t vert_ind = 0; vert_ind < numVerts; ++vert_ind)
  {
//    std::cout << "vert_ind: " << vert_ind << '\n';
    for(auto face_ind : vert_to_faces_map[vert_ind])
    {
//      std::cout << "\tface_ind: " << face_ind << '\n';
      for(auto face_ind_to_cmp : vert_to_faces_map[vert_ind])
      {
        // Ignore self.
        if(face_ind == face_ind_to_cmp)
        {
          continue;
        }
        // Skip if already in map.
        if(angles_between_faces.count(face_ind) &&
           angles_between_faces[face_ind].count(face_ind_to_cmp))
        {
          continue;
        }
        double angle =
          vector_angle(faces[face_ind].norm, faces[face_ind_to_cmp].norm);
        angles_between_faces[face_ind][face_ind_to_cmp] = angle;
        angles_between_faces[face_ind_to_cmp][face_ind] = angle;
//        std::cout << "\t\tangles_between_faces"
//          "[" << face_ind << "][" << face_ind_to_cmp << "]: " <<
//          angles_between_faces[face_ind][face_ind_to_cmp] << '\n';
//        std::cout << "\t\tangles_between_faces"
//          "[" << face_ind_to_cmp << "][" << face_ind << "]: " <<
//          angles_between_faces[face_ind_to_cmp][face_ind] << '\n';
      }
    }
  }
//  std::cout << "---------------------------------------------" << '\n';

  // Using angles between faces to determine
  // if vertices between faces should be smooth.
//  std::cout << "Using angles between faces to determine "
//    "if vertices between faces should be smooth." << '\n';
  std::unordered_map<std::size_t, std::unordered_set<std::size_t>>
    smooth_faces;
  for(auto angles_map_per_face : angles_between_faces)
  {
//    std::cout << "face_ind: " <<
//      angles_map_per_face.first << '\n';
    for(auto face_angle_pair : angles_map_per_face.second)
    {
//      std::cout << "\tface_to_cmp_ind: " <<
//        face_angle_pair.first << '\n';
//      std::cout << "\tangle: " <<
//        face_angle_pair.second << '\n';
      if(face_angle_pair.second - max_smooth_angle < distinct_distance)
      {
        smooth_faces[angles_map_per_face.first].insert(face_angle_pair.first);
//        std::cout << "smooth" << '\n';
      }
//      std::cout << '\n';
    }
  }
//  std::cout << "---------------------------------------------" << '\n';



  // https://www.bytehazard.com/articles/vertnorm.html
  // Accumulating normals in each face.
//  std::cout << "Accumulating normals in each face." << '\n';
  for(std::size_t face_ind = 0; face_ind < numFaces; ++face_ind)
  {
//    std::cout << "face_ind: " << face_ind << '\n';
    face &cur_face = faces[face_ind];
    for(std::size_t vert_f_ind = 0; vert_f_ind < numVertsPerPoly; ++vert_f_ind)
    {
//      std::cout << "\tvert_f_ind: " << vert_f_ind << '\n';
      std::size_t vert_ind = cur_face.verts[vert_f_ind];
      std::size_t norm_ind = cur_face.vertNorms[vert_f_ind];
      std::vector<double> &cur_norm = raw_vertNorms[norm_ind];
      // No need to get weight per surface since non-normalized
      // face norm length is directly proportional to surface area of face.
      cur_norm = vector_multiply(raw_face_norms[face_ind],
                                 vert_angles[face_ind][vert_f_ind]);

//      std::cout << "\tbegin_norm: " <<
//        cur_norm[0] << " " << cur_norm[1] << " " << cur_norm[2] << '\n';

      for(auto face_to_cmp_ind : vert_to_faces_map[vert_ind])
      {
//        std::cout << "\t\tface_to_cmp_ind: " << face_to_cmp_ind << '\n';

        // Ignore self.
        if(face_ind == face_to_cmp_ind)
        {
          continue;
        }

        // Criteria for hard-edges.
        if(!(smooth_faces.count(face_ind) &&
             smooth_faces[face_ind].count(face_to_cmp_ind)))
        {
          continue;
        }

        // Accumulate normal.
        std::size_t vert_to_cmp_f_ind =
          vert_to_faces_map_with_vert_f_num[vert_ind][face_to_cmp_ind];
        // No need to get weight per surface since non-normalized
        // face norm length is directly proportional to surface area of face.
        std::vector<double> norm_to_add =
          vector_multiply(raw_face_norms[face_to_cmp_ind],
                          vert_angles[face_to_cmp_ind][vert_to_cmp_f_ind]);
        vector_plus_self(cur_norm, norm_to_add);

//        std::vector<double> cout_test =
//          vector_multiply(raw_face_norms[face_to_cmp_ind],
//                          vert_angles[face_to_cmp_ind][vert_to_cmp_f_ind]);
//        std::cout << "\t\tnorm_to_accumulate: " <<
//          norm_to_add[0] << " " <<
//          norm_to_add[1] << " " <<
//          norm_to_add[2] << '\n';
//        std::cout << "\t\tcur_norm: " <<
//          cur_norm[0] << " " << cur_norm[1] << " " << cur_norm[2] << '\n';
      }
//    std::cout << "\tend_norm: " <<
//      cur_norm[0] << " " << cur_norm[1] << " " << cur_norm[2] << '\n';
    }
//    std::cout << '\n';
  }
//  std::cout << "---------------------------------------------" << '\n';





  // Normalizing vertex normals.
//  std::cout << "Normalizing vertex normals." << '\n';
  for(auto &&raw_vertNorm : raw_vertNorms)
  {
    vector_scale_self(vector_scale_val, raw_vertNorm);
//    std::cout << "normalized: " <<
//      raw_vertNorm[0] << " " <<
//      raw_vertNorm[1] << " " <<
//      raw_vertNorm[2] << '\n';
  }
//  std::cout << "---------------------------------------------" << '\n';



  // Reducing number of normals.

  // Getting groups of indicies of equal normals.
  std::vector<std::size_t> raw_norm_ids(raw_vertNorms.size());
  std::iota(raw_norm_ids.begin(), raw_norm_ids.end(), 0);

  std::vector<std::vector<std::size_t>> equal_normals_groups =
    get_groups_of_connected_items<std::size_t>(
      raw_norm_ids,
      [&](std::size_t first, std::size_t second)->bool
      {
        return vector_equal(raw_vertNorms[first], raw_vertNorms[second]);
      });

  numVertNorms = equal_normals_groups.size();
  vertNorms = std::vector<std::vector<double>>(numVertNorms,
                                               std::vector<double>(3, 0.0));

  // Getting end normal as average of equal normals.
  for(std::size_t norm_ind = 0; norm_ind < numVertNorms; ++norm_ind)
  {
    for(const auto &norm_group_ind : equal_normals_groups[norm_ind])
    {
      vector_plus_self(vertNorms[norm_ind], raw_vertNorms[norm_group_ind]);
    }
    vector_divide_self(vertNorms[norm_ind],
                       equal_normals_groups[norm_ind].size());
  }

//  std::cout << "raw_to_end_map" << '\n';
  std::unordered_map<std::size_t, std::size_t> raw_to_end_map;
  for(std::size_t norm_ind = 0; norm_ind < numVertNorms; ++norm_ind)
  {
//  std::cout << "norm_ind: " << norm_ind << '\n';
    for(auto raw_norm_ind : equal_normals_groups[norm_ind])
    {
//    std::cout << "\traw_norm_ind: " << raw_norm_ind << '\n';
      raw_to_end_map[raw_norm_ind] = norm_ind;
    }
//    std::cout << '\n';
  }
//  std::cout << "---------------------------------------------" << '\n';

  for(auto &&face : faces)
  {
    for(auto &&vertNorm : face.vertNorms)
    {
      vertNorm = raw_to_end_map[vertNorm];
    }
  }
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



model_extreme_points polyhedron::get_extreme_points(
  const std::vector<const volInt::face*> &polygons_arg) const
{
  model_extreme_points tmp_extreme_points;

  std::vector<const std::vector<double>*> tmp_verts =
    get_vertices_by_polygons(polygons_arg);

  tmp_extreme_points.get_most_extreme(tmp_verts);

  return tmp_extreme_points;
}





std::vector<double> polyhedron::get_model_center()
{
  get_extreme_points();
  return extreme_points.get_center();
}



std::vector<double> polyhedron::get_model_center(
  const std::vector<const std::vector<double>*> &vertices) const
{
  model_extreme_points tmp_extreme_points;
  tmp_extreme_points.get_most_extreme(vertices);
  return tmp_extreme_points.get_center();
}



std::vector<double> polyhedron::get_model_center(
  const std::vector<const volInt::face*> &polygons_arg) const
{
  model_extreme_points tmp_extreme_points = get_extreme_points(polygons_arg);
  return tmp_extreme_points.get_center();
}





std::vector<const volInt::face*> polyhedron::get_polygons_by_color(
  unsigned int color_id_arg) const
{
  std::vector<const volInt::face*> tmp_polygons;
  tmp_polygons.reserve(numFaces);
  for(int cur_poly = 0; cur_poly < numFaces; ++cur_poly)
  {
    if(faces[cur_poly].color_id == color_id_arg)
    {
      tmp_polygons.push_back(&faces[cur_poly]);
    }
  }
  return tmp_polygons;
}



std::vector<const volInt::face*> polyhedron::get_polygons_by_ids(
  unsigned int color_id_arg,
  int wheel_weapon_id_arg) const
{
  std::vector<const volInt::face*> tmp_polygons;
  tmp_polygons.reserve(numFaces);
  for(int cur_poly = 0; cur_poly < numFaces; ++cur_poly)
  {
    if(faces[cur_poly].color_id        == color_id_arg &&
       faces[cur_poly].wheel_weapon_id == wheel_weapon_id_arg)
    {
      tmp_polygons.push_back(&faces[cur_poly]);
    }
  }
  return tmp_polygons;
}





std::vector<const std::vector<double>*> polyhedron::get_vertices_by_polygons(
    const std::vector<const volInt::face*> &polygons_arg) const
{
  std::vector<const std::vector<double>*> tmp_verts;

  // Getting vertices of specified polygons without duplicates.
  std::unordered_set<int> tmp_verts_ids;
  for(const auto polygon : polygons_arg)
  {
    for(auto vert_num : polygon->verts)
    {
      tmp_verts_ids.insert(vert_num);
    }
  }

  tmp_verts.reserve(tmp_verts_ids.size());
  for(auto tmp_vert_id : tmp_verts_ids)
  {
    tmp_verts.push_back(&verts[tmp_vert_id]);
  }

  return tmp_verts;
}



std::vector<const std::vector<double>*> polyhedron::get_vertices_by_color(
  unsigned int color_id) const
{
  std::vector<const volInt::face*> tmp_polygons =
    get_polygons_by_color(color_id);
  return get_vertices_by_polygons(tmp_polygons);
}



std::vector<const std::vector<double>*> polyhedron::get_vertices_by_ids(
  unsigned int color_id,
  int wheel_weapon_id) const
{
  std::vector<const volInt::face*> tmp_polygons =
    get_polygons_by_ids(color_id, wheel_weapon_id);
  return get_vertices_by_polygons(tmp_polygons);
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



void polyhedron::move_model_to_point(
  std::vector<const std::vector<double>*> verts_arg,
  const std::vector<double> &point_arg)
{
  for(auto vert_ptr : verts_arg)
  {
    // Cast from constant pointer to non-constant pointer
    // to modify non-constant "this".
    vector_plus_self(*const_cast<std::vector<double>*>(vert_ptr), point_arg);
  }
}



void polyhedron::move_model_to_point(
  std::vector<const volInt::face*> polygons_arg,
  const std::vector<double> &point_arg)
{
  std::vector<const std::vector<double>*> tmp_verts =
    get_vertices_by_polygons(polygons_arg);
  move_model_to_point(tmp_verts, point_arg);
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



void polyhedron::move_coord_system_to_point(
  std::vector<const std::vector<double>*> verts_arg,
  const std::vector<double> &point_arg)
{
  for(auto vert_ptr : verts_arg)
  {
    // Cast from constant pointer to non-constant pointer
    // to modify non-constant "this".
    vector_minus_self(*const_cast<std::vector<double>*>(vert_ptr), point_arg);
  }
}



void polyhedron::move_coord_system_to_point(
  std::vector<const volInt::face*> polygons_arg,
  const std::vector<double> &point_arg)
{
  std::vector<const std::vector<double>*> tmp_verts =
    get_vertices_by_polygons(polygons_arg);
  move_coord_system_to_point(tmp_verts, point_arg);
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



// Getting two points which are distant enough from each other by x and z.
bool polyhedron::find_ref_points()
{
  int verts_per_poly = faces[0].numVerts;


  for(int cur_poly_one_num = 0;
      cur_poly_one_num < numFaces;
      ++cur_poly_one_num)
  {
    for(int cur_vert_one_num = 0;
        cur_vert_one_num < verts_per_poly;
        ++cur_vert_one_num)
    {
      ref_vert_one_ind.first = cur_poly_one_num;
      ref_vert_one_ind.second = cur_vert_one_num;
      ref_vert_one =
        &verts.at(
          faces[ref_vert_one_ind.first].verts[ref_vert_one_ind.second]);

      // Finding second vert which is far enough from first one by x or z.
      // It's needed to find rotation angle around y axis
      // to get rotation of weapon.
      for(int cur_poly_two_num = 0;
          cur_poly_two_num < numFaces;
          ++cur_poly_two_num)
      {
        for(int cur_vert_two_num = 0;
            cur_vert_two_num < verts_per_poly;
            ++cur_vert_two_num)
        {
          const std::vector<double> *cur_point =
            &verts.at(faces[cur_poly_two_num].verts[cur_vert_two_num]);

          // TEST
          /*
          std::cout << "\n\n\n";
          std::cout << "cur_poly_two_num: " << cur_poly_two_num <<
            ", cur_vert_two_num: " << cur_vert_two_num << '\n';
          std::cout << '\n';
          */

          std::vector<double> rel_to_one =
            vector_minus((*cur_point), (*ref_vert_one));

          if((std::abs(rel_to_one[0]) > distinct_distance ||
                std::abs(rel_to_one[2]) > distinct_distance) &&
             std::abs(rel_to_one[1]) > distinct_distance)
          {
            ref_vert_two_ind.first = cur_poly_two_num;
            ref_vert_two_ind.second = cur_vert_two_num;
            ref_vert_two = cur_point;

            // Finding third vert which is not collinear
            // to vert one and vert two.
            // Needed to check if the model was rotated by other axes.
            for(int cur_poly_three_num = 0;
                cur_poly_three_num < numFaces;
                ++cur_poly_three_num)
            {
              for(int cur_vert_three_num = 0;
                  cur_vert_three_num < verts_per_poly;
                  ++cur_vert_three_num)
              {
                const std::vector<double> *cur_point =
                  &verts.at(faces[cur_poly_three_num].
                    verts[cur_vert_three_num]);

                double x1 = (*ref_vert_two)[0] - (*ref_vert_one)[0];
                double x2 = (*cur_point)[0] - (*ref_vert_one)[0];
                double y1 = (*ref_vert_two)[1] - (*ref_vert_one)[1];
                double y2 = (*cur_point)[1] - (*ref_vert_one)[1];
                double z1 = (*ref_vert_two)[2] - (*ref_vert_one)[2];
                double z2 = (*cur_point)[2] - (*ref_vert_one)[2];

                // TEST
                /*
                std::cout << "x1: " << x1 << '\n';
                std::cout << "x2: " << x2 << '\n';
                std::cout << "y1: " << y1 << '\n';
                std::cout << "y2: " << y2 << '\n';
                std::cout << "z1: " << z1 << '\n';
                std::cout << "z2: " << z2 << '\n';

                std::cout << "x1*y2 - x2*y1: " << x1*y2 - x2*y1 << '\n';
                std::cout << "x1*z2 - x2*z1: " << x1*z2 - x2*z1 << '\n';
                std::cout << "y1*z2 - y2*z1: " << y1*z2 - y2*z1 << '\n';

                std::cout << "sqr_distinct_distance: " <<
                  sqr_distinct_distance << '\n';
                */

                // If current vert is not collinear with vert one and vert two.
                if(!(std::abs(x1*y2 - x2*y1) < sqr_distinct_distance &&
                     std::abs(x1*z2 - x2*z1) < sqr_distinct_distance &&
                     std::abs(y1*z2 - y2*z1) < sqr_distinct_distance))
                {
                  ref_vert_three_ind.first = cur_poly_three_num;
                  ref_vert_three_ind.second = cur_vert_three_num;
                  ref_vert_three = cur_point;

                  // TEST
//                std::cout << "find_ref_points_end_search_loop!" << '\n';

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

  // TEST
  /*
  std::cout << "\n\n";
  std::cout << "ref_vert_one_ind: " << ref_vert_one_ind.first << ", " <<
    ref_vert_one_ind.second << '\n';
  std::cout << '\n';
  std::cout << "ref_vert_two_ind: " << ref_vert_two_ind.first << ", " <<
    ref_vert_two_ind.second << '\n';
  std::cout << "\n\n\n\n\n";
  */



  if(ref_vert_one_ind.first < 0 || ref_vert_two_ind.first < 0)
  {
    return false;
  }

  // TEST
  /*
  std::cout << "\n\n";
  std::cout << "(*ref_vert_two)[0]: " << (*ref_vert_two)[0] <<
    "; (*ref_vert_one)[0]: " << (*ref_vert_one)[0] << '\n';
  std::cout << '\n';
  std::cout << "(*ref_vert_two)[2]: " << (*ref_vert_two)[2] <<
    "; (*ref_vert_one)[2]: " << (*ref_vert_one)[2] << '\n';
  std::cout << "\n\n\n\n\n";

  std::cout << "\n\n";
  std::cout << "((*ref_vert_two)[0] - (*ref_vert_one)[0]): " <<
                ((*ref_vert_two)[0] - (*ref_vert_one)[0]) << '\n';
  std::cout << '\n';
  std::cout << "((*ref_vert_two)[2] - (*ref_vert_one)[2]): " <<
                ((*ref_vert_two)[2] - (*ref_vert_one)[2]) << '\n';
  std::cout << "\n\n\n\n\n";
  */

  ref_vert_two_rel_to_one = vector_minus((*ref_vert_two), (*ref_vert_one));
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

  // TEST
  /*
  if(wheel_id >= 0)
  {
    std::cout << "Wheel " << std::to_string(wheel_id + 1) <<
                 " of input model " << wavefront_obj_path << '\n';
  }
  else
  {
    std::cout << "Input model " + wavefront_obj_path << '\n';
  }
  std::cout << "T0: " << T0 << '\n';
  */

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
    err_msg.append(" have negative volume: \"" + std::to_string(T0) + "\" " +
                   "(before scaling to the in-game units of measurement).");
    throw exception::negative_volume(err_msg);
  }
  // If volume is 0 for main model of *.m3d or first model of *.a3d,
  // the game will freeze while loading models.
  if(T0 == 0.0 && wheel_id < 0)
  {
    throw exception::zero_volume("Input model " + wavefront_obj_path +
                                 " have zero volume.");
  }

  if(!volume_overwritten)
  {
    volume = T0;
  }
  double mass = density * volume;

  if(!rcm_overwritten)
  {
    /* compute center of mass */
    rcm[VOLINT_X] = T1[VOLINT_X] / volume;
    rcm[VOLINT_Y] = T1[VOLINT_Y] / volume;
    rcm[VOLINT_Z] = T1[VOLINT_Z] / volume;
  }

  if(!J_overwritten)
  {
    /* compute inertia tensor */
    J[VOLINT_X][VOLINT_X] = density * (T2[VOLINT_Y] + T2[VOLINT_Z]);
    J[VOLINT_Y][VOLINT_Y] = density * (T2[VOLINT_Z] + T2[VOLINT_X]);
    J[VOLINT_Z][VOLINT_Z] = density * (T2[VOLINT_X] + T2[VOLINT_Y]);
    J[VOLINT_X][VOLINT_Y] = J[VOLINT_Y][VOLINT_X] = -density * TP[VOLINT_X];
    J[VOLINT_Y][VOLINT_Z] = J[VOLINT_Z][VOLINT_Y] = -density * TP[VOLINT_Y];
    J[VOLINT_Z][VOLINT_X] = J[VOLINT_X][VOLINT_Z] = -density * TP[VOLINT_Z];

    /* translate inertia tensor to center of mass */
    J[VOLINT_X][VOLINT_X] -= mass * (rcm[VOLINT_Y] * rcm[VOLINT_Y] +
                             rcm[VOLINT_Z] * rcm[VOLINT_Z]);
    J[VOLINT_Y][VOLINT_Y] -= mass * (rcm[VOLINT_Z] * rcm[VOLINT_Z] +
                             rcm[VOLINT_X] * rcm[VOLINT_X]);
    J[VOLINT_Z][VOLINT_Z] -= mass * (rcm[VOLINT_X] * rcm[VOLINT_X] +
                             rcm[VOLINT_Y] * rcm[VOLINT_Y]);
    J[VOLINT_X][VOLINT_Y] = J[VOLINT_Y][VOLINT_X] +=
      mass * rcm[VOLINT_X] * rcm[VOLINT_Y];
    J[VOLINT_Y][VOLINT_Z] = J[VOLINT_Z][VOLINT_Y] +=
      mass * rcm[VOLINT_Y] * rcm[VOLINT_Z];
    J[VOLINT_Z][VOLINT_X] = J[VOLINT_X][VOLINT_Z] +=
      mass * rcm[VOLINT_Z] * rcm[VOLINT_X];
  }



  // TEST
  /*
  std::cout << '\n';

  std::cout << "xmax: " << xmax() << '\n';
  std::cout << "ymax: " << ymax() << '\n';
  std::cout << "zmax: " << zmax() << '\n';

  std::cout << "xmin: " << xmin() << '\n';
  std::cout << "ymin: " << ymin() << '\n';
  std::cout << "zmin: " << zmin() << '\n';

  std::cout << "rmax: " << rmax << '\n';
  std::cout << "volume: " << volume << '\n';
  std::cout << "rcm: " << rcm[0] << ", " << rcm[1] << ", " << rcm[2] << '\n';
  std::cout << "J: " << '\n' <<
    J[0][0] << ", " << J[0][1] << ", " << J[0][2] << '\n' <<
    J[1][0] << ", " << J[1][1] << ", " << J[1][2] << '\n' <<
    J[2][0] << ", " << J[2][1] << ", " << J[2][2] << '\n';

  std::cout << "\n\n";
  */




  // TEST
  /*
  double expected_volume = 0.0;

  for(int cur_poly = 0; cur_poly < numFaces; ++cur_poly)
  {
    if(numVertsPerPoly == 3)
    {
      std::vector<double> U = vector_minus(verts[faces[cur_poly].verts[1]],
                                           verts[faces[cur_poly].verts[0]]);
      std::vector<double> V = vector_minus(verts[faces[cur_poly].verts[2]],
                                           verts[faces[cur_poly].verts[0]]);


      std::vector<double> flat_normal(3, 0.0);
      flat_normal[0] = U[1] * V[2] - U[2] * V[1];
      flat_normal[1] = U[2] * V[0] - U[0] * V[2];
      flat_normal[2] = U[0] * V[1] - U[1] * V[0];
      flat_normal = vector_scale(1.0, flat_normal);

      std::cout << "expected flat_normal poly " << cur_poly << ": " <<
        flat_normal[0] << ", " <<
        flat_normal[1] << ", "  << 
        flat_normal[2] << '\n';
      std::cout << "flat_normal poly " << cur_poly << ": " <<
        faces[cur_poly].norm[0] << ", " <<
        faces[cur_poly].norm[1] << ", "  << 
        faces[cur_poly].norm[2] << '\n';
    }


      expected_volume +=
        vector_dot_product(
          verts[faces[cur_poly].verts[0]],
          vector_cross_product(verts[faces[cur_poly].verts[1]],
                               verts[faces[cur_poly].verts[2]]))
        /
        6.0f;
//    faces[cur_poly];
    }
  }
  */

  // TEST
  /*
  if(numVertsPerPoly == 3)
  {
    std::cout << "volume: " << volume << '\n';
    std::cout << "expected_volume: " << expected_volume << '\n';
    std::cout << "volume/expected_volume: " << volume/expected_volume << '\n';
    std::cout << "expected_volume/volume: " << expected_volume/volume << '\n';
  }
  */

  // TEST
  /*
//if(false)
  if(numVertsPerPoly == 3)
  {
    DBV p1 = DBV(variable_polygons[i].vertices[2]->x_8,
                 variable_polygons[i].vertices[2]->y_8,
                 variable_polygons[i].vertices[2]->z_8);
    DBV p2 = DBV(variable_polygons[i].vertices[1]->x_8,
                 variable_polygons[i].vertices[1]->y_8,
                 variable_polygons[i].vertices[1]->z_8);
    DBV p3 = DBV(variable_polygons[i].vertices[0]->x_8,
                 variable_polygons[i].vertices[0]->y_8,
                 variable_polygons[i].vertices[0]->z_8);

    DBV U(p2 - p1);
    DBV V(p3 - p1);
    model_debug_info << "U.x" << U.x << '\n';
    model_debug_info << "U.y" << U.y << '\n';
    model_debug_info << "U.z" << U.z << '\n';
    model_debug_info << "V.x" << V.x << '\n';
    model_debug_info << "V.y" << V.y << '\n';
    model_debug_info << "V.z" << V.z << '\n';

    flat_normal.x = U.y * V.z - U.z * V.y;
    flat_normal.y = U.z * V.x - U.x * V.z;
    flat_normal.z = U.x * V.y - U.y * V.x;


    expected_volume += (p1 * (p2 % p3)) / 6.0f;
  }
//else
  else if(numVertsPerPoly == 4)
  {
    DBV p1 = DBV(variable_polygons[i].vertices[3]->x_8,
                 variable_polygons[i].vertices[3]->y_8,
                 variable_polygons[i].vertices[3]->z_8);
    DBV p2 = DBV(variable_polygons[i].vertices[2]->x_8,
                 variable_polygons[i].vertices[2]->y_8,
                 variable_polygons[i].vertices[2]->z_8);
    DBV p3 = DBV(variable_polygons[i].vertices[1]->x_8,
                 variable_polygons[i].vertices[1]->y_8,
                 variable_polygons[i].vertices[1]->z_8);
    DBV p4 = DBV(variable_polygons[i].vertices[0]->x_8,
                 variable_polygons[i].vertices[0]->y_8,
                 variable_polygons[i].vertices[0]->z_8);



    DBV normal(0,0,0);

    for(j = 0;j < num;j++)
    {
      Vertex* current = variable_polygons[i].vertices[num-1-j];
      Vertex* next = variable_polygons[i].vertices[num-1-(j+1)%num];
      flat_normal.x = flat_normal.x + (current -> y_8 - next -> y_8) *
                      (current -> z_8 + next -> z_8);
      flat_normal.y = flat_normal.y + (current -> z_8 - next -> z_8) *
                      (current -> x_8 + next -> x_8);
      flat_normal.z = flat_normal.z + (current -> x_8 - next -> x_8) *
                      (current -> y_8 + next -> y_8);
    }



    expected_volume += (p1 * (p2 % p3)) / 6.0f;
    expected_volume += (p1 * (p3 % p4)) / 6.0f;
  }
  */
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



/*
   ============================================================================
   read in a polyhedron
   ============================================================================
*/

/*
void readPolyhedron(char *name, POLYHEDRON *p)
{
  FILE *fp;
  char line[200], *c;
  int i, j, n;
  double dx1, dy1, dz1, dx2, dy2, dz2, nx, ny, nz, len;
  FACE *f;

  
  if (!(fp = fopen(name, "r"))) {
    printf("i/o error\n");
    exit(1);
  }
  
  fscanf(fp, "%d", &p->numVerts);
  printf("Reading in %d vertices\n", p->numVerts);
  for (i = 0; i < p->numVerts; i++)
    fscanf(fp, "%lf %lf %lf", 
           &p->verts[i][VOLINT_X], &p->verts[i][VOLINT_Y], &p->verts[i][VOLINT_Z]);

  fscanf(fp, "%d", &p->numFaces);
  printf("Reading in %d faces\n", p->numFaces);
  for (i = 0; i < p->numFaces; i++) {
    f = &p->faces[i];
    f->poly = p;
    fscanf(fp, "%d", &f->numVerts);
    for (j = 0; j < f->numVerts; j++) fscanf(fp, "%d", &f->verts[j]);

//     compute face normal and offset w from first 3 vertices 
    dx1 = p->verts[f->verts[1]][VOLINT_X] - p->verts[f->verts[0]][VOLINT_X];
    dy1 = p->verts[f->verts[1]][VOLINT_Y] - p->verts[f->verts[0]][VOLINT_Y];
    dz1 = p->verts[f->verts[1]][VOLINT_Z] - p->verts[f->verts[0]][VOLINT_Z];
    dx2 = p->verts[f->verts[2]][VOLINT_X] - p->verts[f->verts[1]][VOLINT_X];
    dy2 = p->verts[f->verts[2]][VOLINT_Y] - p->verts[f->verts[1]][VOLINT_Y];
    dz2 = p->verts[f->verts[2]][VOLINT_Z] - p->verts[f->verts[1]][VOLINT_Z];
    nx = dy1 * dz2 - dy2 * dz1;
    ny = dz1 * dx2 - dz2 * dx1;
    nz = dx1 * dy2 - dx2 * dy1;
    len = sqrt(nx * nx + ny * ny + nz * nz);
    f->norm[VOLINT_X] = nx / len;
    f->norm[VOLINT_Y] = ny / len;
    f->norm[VOLINT_Z] = nz / len;
    f->w = - f->norm[VOLINT_X] * p->verts[f->verts[0]][VOLINT_X]
           - f->norm[VOLINT_Y] * p->verts[f->verts[0]][VOLINT_Y]
           - f->norm[VOLINT_Z] * p->verts[f->verts[0]][VOLINT_Z];

  }

  fclose(fp);

}
*/

/*
   ============================================================================
   compute mass properties
   ============================================================================
*/


/* compute various integrations over projection of face */
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

  for (i = 0; i < f->numVerts; i++) {
    a0 = p->verts[f->verts[i]][A];
    b0 = p->verts[f->verts[i]][B];
    a1 = p->verts[f->verts[(i+1) % f->numVerts]][A];
    b1 = p->verts[f->verts[(i+1) % f->numVerts]][B];
    da = a1 - a0;
    db = b1 - b0;
    a0_2 = a0 * a0; a0_3 = a0_2 * a0; a0_4 = a0_3 * a0;
    b0_2 = b0 * b0; b0_3 = b0_2 * b0; b0_4 = b0_3 * b0;
    a1_2 = a1 * a1; a1_3 = a1_2 * a1; 
    b1_2 = b1 * b1; b1_3 = b1_2 * b1;

    C1 = a1 + a0;
    Ca = a1*C1 + a0_2; Caa = a1*Ca + a0_3; Caaa = a1*Caa + a0_4;
    Cb = b1*(b1 + b0) + b0_2; Cbb = b1*Cb + b0_3; Cbbb = b1*Cbb + b0_4;
    Cab = 3*a1_2 + 2*a1*a0 + a0_2; Kab = a1_2 + 2*a1*a0 + 3*a0_2;
    Caab = a0*Cab + 4*a1_3; Kaab = a1*Kab + 4*a0_3;
    Cabb = 4*b1_3 + 3*b1_2*b0 + 2*b1*b0_2 + b0_3;
    Kabb = b1_3 + 2*b1_2*b0 + 3*b1*b0_2 + 4*b0_3;

    P1 += db*C1;
    Pa += db*Ca;
    Paa += db*Caa;
    Paaa += db*Caaa;
    Pb += da*Cb;
    Pbb += da*Cbb;
    Pbbb += da*Cbbb;
    Pab += db*(b1*Cab + b0*Kab);
    Paab += db*(b1*Caab + b0*Kaab);
    Pabb += da*(a1*Cabb + a0*Kabb);
  }

  P1 /= 2.0;
  Pa /= 6.0;
  Paa /= 12.0;
  Paaa /= 20.0;
  Pb /= -6.0;
  Pbb /= -12.0;
  Pbbb /= -20.0;
  Pab /= 24.0;
  Paab /= 60.0;
  Pabb /= -60.0;
}

void compFaceIntegrals(POLYHEDRON *p, FACE *f)
{
//  double *n, w;
  std::vector<double> n;
  double w;
  double k1, k2, k3, k4;

  compProjectionIntegrals(p, f);

  w = f->w;
  n = f->norm;
  k1 = 1 / n[C]; k2 = k1 * k1; k3 = k2 * k1; k4 = k3 * k1;

  Fa = k1 * Pa;
  Fb = k1 * Pb;
  Fc = -k2 * (n[A]*Pa + n[B]*Pb + w*P1);

  Faa = k1 * Paa;
  Fbb = k1 * Pbb;
  Fcc = k3 * (VOLINT_SQR(n[A])*Paa + 2*n[A]*n[B]*Pab + VOLINT_SQR(n[B])*Pbb
         + w*(2*(n[A]*Pa + n[B]*Pb) + w*P1));

  Faaa = k1 * Paaa;
  Fbbb = k1 * Pbbb;
  Fccc = -k4 * (VOLINT_CUBE(n[A])*Paaa + 3*VOLINT_SQR(n[A])*n[B]*Paab 
           + 3*n[A]*VOLINT_SQR(n[B])*Pabb + VOLINT_CUBE(n[B])*Pbbb
           + 3*w*(VOLINT_SQR(n[A])*Paa + 2*n[A]*n[B]*Pab + VOLINT_SQR(n[B])*Pbb)
           + w*w*(3*(n[A]*Pa + n[B]*Pb) + w*P1));

  Faab = k1 * Paab;
  Fbbc = -k2 * (n[A]*Pabb + n[B]*Pbbb + w*Pbb);
  Fcca = k3 * (VOLINT_SQR(n[A])*Paaa + 2*n[A]*n[B]*Paab + VOLINT_SQR(n[B])*Pabb
         + w*(2*(n[A]*Paa + n[B]*Pab) + w*Pa));
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

    T0 += f->norm[VOLINT_X] * ((A == VOLINT_X) ? Fa : ((B == VOLINT_X) ? Fb : Fc));

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


/*
   ============================================================================
   main
   ============================================================================
*/



//int main(int argc, char *argv[])
//{
//  POLYHEDRON p;
//  double density, mass;
//  double r[3];            /* center of mass */
//  double J[3][3];         /* inertia tensor */
//
//  if (argc != 2) {
//    printf("usage:  %s <polyhedron geometry filename>\n", argv[0]);
//    exit(0);
//  }
//
//  readPolyhedron(argv[1], &p);

//  compVolumeIntegrals(&p);
//
//
//  printf("\nT1 =   %+20.6f\n\n", T0);
//
//  printf("Tx =   %+20.6f\n", T1[VOLINT_X]);
//  printf("Ty =   %+20.6f\n", T1[VOLINT_Y]);
//  printf("Tz =   %+20.6f\n\n", T1[VOLINT_Z]);
//  
//  printf("Txx =  %+20.6f\n", T2[VOLINT_X]);
//  printf("Tyy =  %+20.6f\n", T2[VOLINT_Y]);
//  printf("Tzz =  %+20.6f\n\n", T2[VOLINT_Z]);
//
//  printf("Txy =  %+20.6f\n", TP[VOLINT_X]);
//  printf("Tyz =  %+20.6f\n", TP[VOLINT_Y]);
//  printf("Tzx =  %+20.6f\n\n", TP[VOLINT_Z]);
//
//  density = 1.0;  /* assume unit density */
//
//  mass = density * T0;
//
//  /* compute center of mass */
//  r[VOLINT_X] = T1[VOLINT_X] / T0;
//  r[VOLINT_Y] = T1[VOLINT_Y] / T0;
//  r[VOLINT_Z] = T1[VOLINT_Z] / T0;
//
//  /* compute inertia tensor */
//  J[VOLINT_X][VOLINT_X] = density * (T2[VOLINT_Y] + T2[VOLINT_Z]);
//  J[VOLINT_Y][VOLINT_Y] = density * (T2[VOLINT_Z] + T2[VOLINT_X]);
//  J[VOLINT_Z][VOLINT_Z] = density * (T2[VOLINT_X] + T2[VOLINT_Y]);
//  J[VOLINT_X][VOLINT_Y] = J[VOLINT_Y][VOLINT_X] = - density * TP[VOLINT_X];
//  J[VOLINT_Y][VOLINT_Z] = J[VOLINT_Z][VOLINT_Y] = - density * TP[VOLINT_Y];
//  J[VOLINT_Z][VOLINT_X] = J[VOLINT_X][VOLINT_Z] = - density * TP[VOLINT_Z];
//
//  /* translate inertia tensor to center of mass */
//  J[VOLINT_X][VOLINT_X] -= mass * (r[VOLINT_Y]*r[VOLINT_Y] + r[VOLINT_Z]*r[VOLINT_Z]);
//  J[VOLINT_Y][VOLINT_Y] -= mass * (r[VOLINT_Z]*r[VOLINT_Z] + r[VOLINT_X]*r[VOLINT_X]);
//  J[VOLINT_Z][VOLINT_Z] -= mass * (r[VOLINT_X]*r[VOLINT_X] + r[VOLINT_Y]*r[VOLINT_Y]);
//  J[VOLINT_X][VOLINT_Y] = J[VOLINT_Y][VOLINT_X] += mass * r[VOLINT_X] * r[VOLINT_Y]; 
//  J[VOLINT_Y][VOLINT_Z] = J[VOLINT_Z][VOLINT_Y] += mass * r[VOLINT_Y] * r[VOLINT_Z]; 
//  J[VOLINT_Z][VOLINT_X] = J[VOLINT_X][VOLINT_Z] += mass * r[VOLINT_Z] * r[VOLINT_X]; 
//
//  printf("center of mass:  (%+12.6f,%+12.6f,%+12.6f)\n\n", r[VOLINT_X], r[VOLINT_Y], r[VOLINT_Z]);
//
//  printf("inertia tensor with origin at c.o.m. :\n");
//  printf("%+15.6f  %+15.6f  %+15.6f\n", J[VOLINT_X][VOLINT_X], J[VOLINT_X][VOLINT_Y], J[VOLINT_X][VOLINT_Z]);
//  printf("%+15.6f  %+15.6f  %+15.6f\n", J[VOLINT_Y][VOLINT_X], J[VOLINT_Y][VOLINT_Y], J[VOLINT_Y][VOLINT_Z]);
//  printf("%+15.6f  %+15.6f  %+15.6f\n\n", J[VOLINT_Z][VOLINT_X], J[VOLINT_Z][VOLINT_Y], J[VOLINT_Z][VOLINT_Z]);
//  
//}



} // namespace volInt
