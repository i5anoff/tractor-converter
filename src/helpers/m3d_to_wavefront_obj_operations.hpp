#ifndef TRACTOR_CONVERTER_M3D_TO_WAVEFRONT_OBJ_OPERATIONS_H
#define TRACTOR_CONVERTER_M3D_TO_WAVEFRONT_OBJ_OPERATIONS_H

#include "defines.hpp"
#include "bitflag.hpp"
#include "wavefront_obj_constants.hpp"
#include "vangers_3d_model_constants.hpp"

#include "raw_num_operations.hpp"
#include "file_operations.hpp"
#include "vangers_cfg_operations.hpp"
#include "wavefront_obj_operations.hpp"
#include "vangers_3d_model_operations.hpp"

#include "volInt.hpp"
#include "tiny_obj_loader.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <exception>
#include <stdexcept>

#include <cstdint>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <utility>
#include <iterator>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>



namespace tractor_converter{
namespace helpers{



enum class m3d_to_obj_flag
{
  none = 0,
  extract_bound_model = 1,
  extract_nonexistent_weapons = 2,
  use_custom_volume_by_default = 3,
  use_custom_rcm_by_default = 4,
  use_custom_J_by_default = 5,
};

// 1 digit left to dot + 1 dot + 8 expected digits right to dot + 4 exponent
const std::size_t per_file_cfg_float_size_increase = 1 + 1 + 8 + 4;
// 19 integer number
const std::size_t per_file_cfg_int_size_increase = 19;



const std::size_t per_file_cfg_expected_main_size = 3000;
const std::size_t per_file_cfg_expected_debris_header_size = 3000;
const std::size_t per_file_cfg_expected_debris_el_size = 1000;
const std::size_t per_file_cfg_expected_animated_header_size = 3000;
const std::size_t per_file_cfg_expected_animated_el_size = 1000;



const std::unordered_set<std::string> inertia_tensor_sensitive_models =
  {
    "f1", // Fish.
  };



class m3d_to_wavefront_obj_model : vangers_model
{
public:

  m3d_to_wavefront_obj_model(
    const boost::filesystem::path &input_m3d_path_arg,
    const boost::filesystem::path &output_m3d_path_arg,
    const std::string &input_file_name_error_arg,
    const std::string &output_file_name_error_arg,
    const volInt::polyhedron *example_weapon_model_arg,
    const volInt::polyhedron *weapon_attachment_point_arg,
    const volInt::polyhedron *ghost_wheel_model_arg,
    const volInt::polyhedron *center_of_mass_model_arg,
    double scale_size_arg,
    unsigned int float_precision_objs_arg,
    bitflag<m3d_to_obj_flag> flags_arg);



  void mechos_m3d_to_wavefront_objs();
  volInt::polyhedron weapon_m3d_to_wavefront_objs();
  void animated_a3d_to_wavefront_objs();
  void other_m3d_to_wavefront_objs();



private:

  unsigned int float_precision_objs;

  bitflag<m3d_to_obj_flag> flags;

  std::string float_precision_objs_string;

  std::size_t expected_medium_vertex_size;
  std::size_t expected_medium_normal_size;

  std::size_t non_steer_ghost_wheels_num;



  boost::filesystem::path file_prefix_to_path(
    const std::string &prefix,
    const std::size_t *model_num = nullptr);



  template<typename SOURCE, typename DESTINATION>
  DESTINATION read_var_from_m3d()
  {
    DESTINATION var = raw_bytes_to_num<SOURCE>(m3d_data, m3d_data_cur_pos);
    m3d_data_cur_pos += sizeof(SOURCE);
    return var;
  }

  // The program must always read floating point numbers in corresponding type.
  // There should be no need for read_var_from_m3d_rounded<>().
  template<typename SOURCE, typename DESTINATION>
  DESTINATION read_var_from_m3d_scaled(double exp = 1.0)
  {
    return scale_trunc<DESTINATION>(
             read_var_from_m3d<SOURCE, DESTINATION>(), exp);
  }


  template<typename SOURCE, typename DESTINATION>
  std::vector<DESTINATION> read_vec_var_from_m3d(std::size_t count)
  {
    std::vector<SOURCE> vec_src =
      raw_bytes_to_vec_num<SOURCE>(m3d_data, m3d_data_cur_pos, count);
    std::vector<DESTINATION> vec_dest =
      cast_vec_var<SOURCE, DESTINATION>(vec_src);
    m3d_data_cur_pos += sizeof(SOURCE) * count;
    return vec_dest;
  }

  template<typename SOURCE, typename DESTINATION>
  std::vector<DESTINATION> read_vec_var_from_m3d_scaled(
    std::size_t count, double exp = 1.0)
  {
    std::vector<DESTINATION> vec =
      read_vec_var_from_m3d<SOURCE, DESTINATION>(count);
    scale_vec_trunc<DESTINATION>(vec, exp);
    return vec;
  }


  template<typename SOURCE, typename DESTINATION>
  std::vector<std::vector<DESTINATION>> read_nest_vec_var_from_m3d(
    const std::vector<std::size_t> &count_map)
  {
    std::vector<std::vector<SOURCE>> nest_vec_src =
      raw_bytes_to_nest_vec_num<SOURCE>(m3d_data, m3d_data_cur_pos, count_map);
    std::vector<std::vector<DESTINATION>> nest_vec_dest =
      cast_nest_vec_var<SOURCE, DESTINATION>(nest_vec_src);
    for(const auto count : count_map)
    {
      m3d_data_cur_pos += sizeof(SOURCE) * count;
    }
    return nest_vec_dest;
  }

  template<typename SOURCE, typename DESTINATION>
  std::vector<std::vector<DESTINATION>> read_nest_vec_var_from_m3d_scaled(
    const std::vector<std::size_t> &count_map, double exp = 1.0)
  {
    std::vector<std::vector<DESTINATION>> nest_vec =
      read_nest_vec_var_from_m3d<SOURCE, DESTINATION>(count_map);
    scale_nest_vec_trunc<DESTINATION>(nest_vec, exp);
    return nest_vec;
  }


  template<typename SOURCE, typename DESTINATION>
  std::vector<std::vector<DESTINATION>> read_nest_vec_var_from_m3d(
    std::size_t count_vec, std::size_t count_el)
  {
    std::vector<std::vector<SOURCE>> nest_vec_src =
      raw_bytes_to_nest_vec_num<SOURCE>(m3d_data,
                                        m3d_data_cur_pos,
                                        count_vec,
                                        count_el);
    std::vector<std::vector<DESTINATION>> nest_vec_dest =
      cast_nest_vec_var<SOURCE, DESTINATION>(nest_vec_src);
    m3d_data_cur_pos += sizeof(SOURCE) * count_vec * count_el;
    return nest_vec_dest;
  }

  template<typename SOURCE, typename DESTINATION>
  std::vector<std::vector<DESTINATION>> read_nest_vec_var_from_m3d_scaled(
    std::size_t count_vec, std::size_t count_el, double exp = 1.0)
  {
    std::vector<std::vector<DESTINATION>> nest_vec =
      read_nest_vec_var_from_m3d<SOURCE, DESTINATION>(count_vec, count_el);
    scale_nest_vec_trunc<DESTINATION>(nest_vec, exp);
    return nest_vec;
  }



  std::vector<double> read_vertex();
  void read_vertices(volInt::polyhedron &model);

  std::vector<double> read_normal(bitflag<normal_flag> flags);
  void read_normals(volInt::polyhedron &model);

  volInt::face read_polygon(const volInt::polyhedron &model,
                            std::size_t cur_poly);
  void read_polygons(volInt::polyhedron &model);

  void read_sorted_polygon_indices(volInt::polyhedron &model);

  volInt::polyhedron read_c3d(c3d::c3d_type cur_c3d_type);


  void save_c3d_as_wavefront_obj(
    std::unordered_map<std::string, volInt::polyhedron> &c3d_models,
    const std::string &prefix,
    const std::size_t *model_num = nullptr);
  void save_c3d_as_wavefront_obj(volInt::polyhedron &c3d_model,
                                 const std::string &prefix,
                                 const std::size_t *model_num = nullptr);
  // Executes "read_c3d" and then "save_c3d_as_wavefront_obj" functions.
  void c3d_to_wavefront_obj(const std::string &prefix,
                            const std::size_t *model_num,
                            c3d::c3d_type cur_c3d_type);
  void read_m3d_header_data();
  void read_a3d_header_data();


  void read_m3d_wheel_data(
    std::vector<volInt::polyhedron> &wheel_models, std::size_t wheel_id);
  std::vector<volInt::polyhedron> read_m3d_wheels_data();



  void mark_wheels(
    volInt::polyhedron &main_model,
    const std::vector<volInt::polyhedron> &steer_wheels_models);

  volInt::polyhedron get_ghost_wheels_helper_generate_wheel(int wheel_id);
  void get_ghost_wheels(
    const volInt::polyhedron &main_model,
    std::vector<volInt::polyhedron> &steer_wheels_models,
    std::vector<volInt::polyhedron> &non_steer_ghost_wheels_models);


  void move_weapon_model(std::vector<double> new_position,
                         double new_angle,
                         int weapon_id,
                         volInt::polyhedron &weapon_model) const;

  void add_weapons_to_models_map(
    std::unordered_map<std::string, volInt::polyhedron> &models_map) const;

  void add_attachment_point_to_models_map(
    std::unordered_map<std::string, volInt::polyhedron> &models_map,
    point attachment_point_pos) const;

  void add_center_of_mass_to_models_map(
    std::unordered_map<std::string, volInt::polyhedron> &models_map,
    point center_of_mass_pos) const;



  void read_m3d_debris_data(
    std::vector<std::unordered_map<std::string, volInt::polyhedron>>
      &debris_models,
    std::vector<volInt::polyhedron> &debris_bound_models,
    std::size_t debris_num);
  void read_m3d_debris_data(
    std::vector<std::unordered_map<std::string, volInt::polyhedron>>
      &debris_models,
    std::vector<volInt::polyhedron> &debris_bound_models);

  void move_debris_to_offset(
    std::unordered_map<std::string, volInt::polyhedron> &debris_model,
    volInt::polyhedron &debris_bound_model);
  void move_debris_to_offset(
    std::vector<std::unordered_map<std::string, volInt::polyhedron>>
      &debris_models,
    std::vector<volInt::polyhedron> &debris_bound_models);

  void save_m3d_debris_data(
    std::vector<std::unordered_map<std::string, volInt::polyhedron>>
      *debris_models,
    std::vector<volInt::polyhedron> *debris_bound_models = nullptr);



  void read_m3d_weapon_slot(std::size_t slot_id);
  void read_m3d_weapon_slots();



  void save_file_cfg_m3d(
    volInt::polyhedron &main_model,
    std::vector<std::unordered_map<std::string, volInt::polyhedron>>
      *debris_models = nullptr);
  void save_file_cfg_a3d(
    std::vector<std::unordered_map<std::string, volInt::polyhedron>>
      &animated_models);
};



std::unordered_map<std::string, double> read_scales_and_copy_game_lst(
  const boost::filesystem::path &lst_filepath,
  const boost::filesystem::path &where_to_save,
  const std::string &input_file_name_error,
  const std::string &output_file_name_error);
double read_scale_and_copy_prm(
  const boost::filesystem::path &prm_filepath,
  const boost::filesystem::path &where_to_save,
  const std::string &input_file_name_error,
  const std::string &output_file_name_error);
void mechos_m3d_to_wavefront_objs(
  const boost::filesystem::path &m3d_filepath,
  const boost::filesystem::path &output_m3d_path,
  const std::string &input_file_name_error,
  const std::string &output_file_name_error,
  const volInt::polyhedron *weapon_model_ptr,
  const volInt::polyhedron *ghost_wheel_model_ptr,
  const volInt::polyhedron *center_of_mass_model_ptr,
  double scale_size,
  unsigned int float_precision_objs,
  bitflag<m3d_to_obj_flag> flags);
volInt::polyhedron weapon_m3d_to_wavefront_objs(
  const boost::filesystem::path &m3d_filepath,
  const boost::filesystem::path &output_m3d_path,
  const std::string &input_file_name_error,
  const std::string &output_file_name_error,
  const volInt::polyhedron *weapon_attachment_point_model_ptr,
  const volInt::polyhedron *center_of_mass_model_ptr,
  double scale_size,
  unsigned int float_precision_objs,
  bitflag<m3d_to_obj_flag> flags);
void animated_a3d_to_wavefront_objs(
  const boost::filesystem::path &a3d_filepath,
  const boost::filesystem::path &output_m3d_path,
  const std::string &input_file_name_error,
  const std::string &output_file_name_error,
  const volInt::polyhedron *center_of_mass_model_ptr,
  double scale_size,
  unsigned int float_precision_objs,
  bitflag<m3d_to_obj_flag> flags);
void other_m3d_to_wavefront_objs(
  const boost::filesystem::path &m3d_filepath,
  const boost::filesystem::path &output_m3d_path,
  const std::string &input_file_name_error,
  const std::string &output_file_name_error,
  const volInt::polyhedron *center_of_mass_model_ptr,
  double scale_size,
  unsigned int float_precision_objs,
  bitflag<m3d_to_obj_flag> flags);



} // namespace helpers
} // namespace tractor_converter

#endif // TRACTOR_CONVERTER_M3D_TO_WAVEFRONT_OBJ_OPERATIONS_H
