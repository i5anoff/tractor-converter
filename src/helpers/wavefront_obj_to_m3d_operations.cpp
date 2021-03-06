#include "wavefront_obj_to_m3d_operations.hpp"



namespace tractor_converter{
namespace helpers{



void get_extreme_radius(double &cur_extreme, double radius, point offset)
{
  double offset_radius = volInt::vector_length(offset);
  double end_radius = std::abs(radius) + offset_radius;
  if(cur_extreme < end_radius)
  {
    cur_extreme = end_radius;
  }
}



// Get radius of bound sphere around attachment point.
double get_weapons_bound_sphere_radius(
  const std::unordered_map<std::string, volInt::polyhedron> &weapons_models)
{
  double max_radius = 0.0;
  for(const auto &weapon_model : weapons_models)
  {
    for(const auto &vert : weapon_model.second.verts)
    {
      double cur_vert_length =
        volInt::vector_length_between(vert,
                                      weapon_model.second.offset_point());
      if(max_radius < cur_vert_length)
      {
        max_radius = cur_vert_length;
      }
    }
  }
  return max_radius;
}



template<>
double parse_per_file_cfg_option<double>(
  const std::string &input)
{
  char* cur_pos = const_cast<char*>(&input[0]);
  return std::strtod(cur_pos, nullptr);
}

template<>
std::vector<double> parse_per_file_cfg_multiple_options<double>(
  const std::string &input)
{
  std::vector<double> to_return;
  char* cur_pos = const_cast<char*>(&input[0]);
  while(*cur_pos != '\0')
  {
    to_return.push_back(std::strtod(cur_pos, &cur_pos));
  }
  return to_return;
}

template<>
std::vector<double> parse_per_file_cfg_multiple_options<double>(
  const std::vector<std::string> &input)
{
  std::vector<double> to_return;
  for(const auto &cur_line : input)
  {
    std::vector<double> per_line_values;
    per_line_values = parse_per_file_cfg_multiple_options<double>(cur_line);
    to_return.insert(to_return.end(),
                     per_line_values.begin(),
                     per_line_values.end());
  }

  return to_return;
}



wavefront_obj_to_m3d_model::wavefront_obj_to_m3d_model(
  const boost::filesystem::path &input_m3d_path_arg,
  const boost::filesystem::path &output_m3d_path_arg,
  const std::string &input_file_name_error_arg,
  const std::string &output_file_name_error_arg,
  const volInt::polyhedron *example_weapon_model_arg,
  const volInt::polyhedron *weapon_attachment_point_arg,
  const volInt::polyhedron *center_of_mass_model_arg,
  double max_weapons_radius_arg,
  unsigned int default_c3d_material_id_arg,
  double scale_cap_arg,
  double max_smooth_angle_arg,
  std::size_t gen_bound_layers_num_arg,
  double gen_bound_area_threshold_arg,
  bitflag<obj_to_m3d_flag> flags_arg,
  std::unordered_map<std::string, double> *non_mechos_scale_sizes_arg)
: vangers_model(
    input_m3d_path_arg,
    output_m3d_path_arg,
    input_file_name_error_arg,
    output_file_name_error_arg,
    example_weapon_model_arg,
    weapon_attachment_point_arg,
    nullptr,
    center_of_mass_model_arg),
  max_weapons_radius(max_weapons_radius_arg),
  default_c3d_material_id(default_c3d_material_id_arg),
  scale_cap(scale_cap_arg),
  max_smooth_angle(max_smooth_angle_arg),
  gen_bound_layers_num(gen_bound_layers_num_arg),
  gen_bound_area_threshold(gen_bound_area_threshold_arg),
  flags(flags_arg),
  non_mechos_scale_sizes(non_mechos_scale_sizes_arg)
{
  model_name = input_m3d_path_arg.filename().string();

  prm_scale_size = 0.0;
}



void wavefront_obj_to_m3d_model::mechos_wavefront_objs_to_m3d()
{
  volInt::polyhedron cur_main_model =
    read_obj_prefix(wavefront_obj::prefix::main,
                    c3d::c3d_type::main_of_mechos);

  volInt::polyhedron cur_main_bound_model;
  volInt::polyhedron *cur_main_bound_model_ptr = nullptr;
  if(!(flags & obj_to_m3d_flag::generate_bound_models))
  {
    cur_main_bound_model =
      read_obj_prefix(wavefront_obj::prefix::main_bound, c3d::c3d_type::bound);
    cur_main_bound_model_ptr = &cur_main_bound_model;
  }


  std::deque<volInt::polyhedron> debris_models =
    read_objs_with_prefix(wavefront_obj::prefix::debris,
                          c3d::c3d_type::regular);

  std::deque<volInt::polyhedron> debris_bound_models;
  std::deque<volInt::polyhedron> *debris_bound_models_ptr = nullptr;
  if(!(flags & obj_to_m3d_flag::generate_bound_models))
  {
    debris_bound_models =
      read_objs_with_prefix(wavefront_obj::prefix::debris_bound,
                            c3d::c3d_type::bound);
    debris_bound_models_ptr = &debris_bound_models;
  }


  // Must be called before call to remove_polygons() and get_m3d_header_data().
  read_file_cfg_m3d(cur_main_model, &debris_models);



  get_weapons_data(cur_main_model);
  get_wheels_data(cur_main_model);
  get_debris_data(&debris_models, debris_bound_models_ptr);

  std::unordered_map<int, volInt::polyhedron> wheels_models =
    get_wheels_steer(cur_main_model);


  // Must be called before call to get_m3d_scale_size().
  remove_polygons(cur_main_model, remove_polygons_model::mechos);
  for(auto &&debris_model : debris_models)
  {
    remove_polygons(debris_model, remove_polygons_model::non_mechos);
  }

  center_debris(&debris_models, debris_bound_models_ptr);

  if(flags & obj_to_m3d_flag::center_model)
  {
    center_m3d(&cur_main_model,
               center_m3d_model::non_weapon,
               cur_main_bound_model_ptr,
               &wheels_models,
               &debris_models,
               debris_bound_models_ptr);
  }

  get_m3d_scale_size(&cur_main_model,
                     cur_main_bound_model_ptr,
                     &wheels_models,
                     &debris_models,
                     debris_bound_models_ptr);

  get_m3d_header_data(&cur_main_model,
                      cur_main_bound_model_ptr,
                      &wheels_models,
                      &debris_models,
                      debris_bound_models_ptr);


  if(flags & obj_to_m3d_flag::generate_bound_models)
  {
    m3d_mechos_generate_bound(
      &cur_main_model,
      &wheels_models,
      &debris_models,
      cur_main_bound_model,
      debris_bound_models);
    cur_main_bound_model_ptr = &cur_main_bound_model;
    debris_bound_models_ptr = &debris_bound_models;
  }

  if(flags & obj_to_m3d_flag::recalculate_vertex_normals)
  {
    m3d_recalc_vertNorms(&cur_main_model,
                         cur_main_bound_model_ptr,
                         &wheels_models,
                         &debris_models,
                         debris_bound_models_ptr);
  }



  std::size_t m3d_file_size = get_m3d_file_size(&cur_main_model,
                                                cur_main_bound_model_ptr,
                                                &wheels_models,
                                                &debris_models,
                                                debris_bound_models_ptr);



  // Allocating enough space for *.m3d file.
  m3d_data = std::string(m3d_file_size, '\0');
  m3d_data_cur_pos = 0;

  write_c3d(cur_main_model);
  write_m3d_header_data();

  if(n_wheels)
  {
    write_m3d_wheels_data(wheels_models);
  }
  if(n_debris)
  {
    write_m3d_debris_data(debris_models, *debris_bound_models_ptr);
  }
  write_c3d(*cur_main_bound_model_ptr);


  write_var_to_m3d<int, std::int32_t>(weapon_slots_existence);
  if(weapon_slots_existence)
  {
    write_m3d_weapon_slots();
  }


  boost::filesystem::path file_to_save = output_m3d_path;
  file_to_save.append(model_name + ext::m3d,
                      boost::filesystem::path::codecvt());
  save_file(file_to_save,
            m3d_data,
            file_flag::binary,
            output_file_name_error);



  boost::filesystem::path prm_file_input = input_m3d_path;
  prm_file_input.append(model_name + ext::prm,
                        boost::filesystem::path::codecvt());

  boost::filesystem::path prm_file_output = output_m3d_path;
  prm_file_output.append(model_name + ext::prm,
                         boost::filesystem::path::codecvt());

  create_prm(prm_file_input,
             prm_file_output,
             input_file_name_error,
             output_file_name_error,
             prm_scale_size);
}



volInt::polyhedron wavefront_obj_to_m3d_model::weapon_wavefront_objs_to_m3d()
{
  volInt::polyhedron cur_main_model =
    read_obj_prefix(wavefront_obj::prefix::main, c3d::c3d_type::regular);

  volInt::polyhedron cur_main_bound_model;
  volInt::polyhedron *cur_main_bound_model_ptr = nullptr;
  if(!(flags & obj_to_m3d_flag::generate_bound_models))
  {
    cur_main_bound_model =
      read_obj_prefix(wavefront_obj::prefix::main_bound, c3d::c3d_type::bound);
    cur_main_bound_model_ptr = &cur_main_bound_model;
  }

  // Must be called before call to remove_polygons() and get_m3d_header_data().
  read_file_cfg_m3d(cur_main_model);



  get_attachment_point(cur_main_model);


  // Must be called before call to get_m3d_scale_size().
  remove_polygons(cur_main_model, remove_polygons_model::non_mechos);

  if(flags & obj_to_m3d_flag::center_model)
  {
    center_m3d(&cur_main_model,
               center_m3d_model::weapon,
               cur_main_bound_model_ptr);
  }

  get_m3d_scale_size(&cur_main_model, cur_main_bound_model_ptr);

  get_m3d_header_data(&cur_main_model, cur_main_bound_model_ptr);


  if(flags & obj_to_m3d_flag::generate_bound_models)
  {
    m3d_non_mechos_generate_bound(&cur_main_model, cur_main_bound_model);
    cur_main_bound_model_ptr = &cur_main_bound_model;
  }

  if(flags & obj_to_m3d_flag::recalculate_vertex_normals)
  {
    m3d_recalc_vertNorms(&cur_main_model, cur_main_bound_model_ptr);
  }



  std::size_t m3d_file_size =
    get_m3d_file_size(&cur_main_model, cur_main_bound_model_ptr);



  // Allocating enough space for *.m3d file.
  m3d_data = std::string(m3d_file_size, '\0');
  m3d_data_cur_pos = 0;

  write_c3d(cur_main_model);
  write_m3d_header_data();
  write_c3d(*cur_main_bound_model_ptr);
  write_var_to_m3d<int, std::int32_t>(weapon_slots_existence);



  boost::filesystem::path file_to_save = output_m3d_path;
  file_to_save.append(model_name + ext::m3d,
                      boost::filesystem::path::codecvt());
  save_file(file_to_save,
            m3d_data,
            file_flag::binary,
            output_file_name_error);

  return cur_main_model;
}



void wavefront_obj_to_m3d_model::animated_wavefront_objs_to_a3d()
{
  std::deque<volInt::polyhedron> animated_models =
    read_objs_with_prefix(wavefront_obj::prefix::animated,
                          c3d::c3d_type::regular);


  // Must be called before call to remove_polygons() and get_a3d_header_data().
  read_file_cfg_a3d(animated_models);



  // Must be called before call to get_a3d_scale_size().
  for(auto &&animated_model : animated_models)
  {
    remove_polygons(animated_model, remove_polygons_model::non_mechos);
  }

  if(flags & obj_to_m3d_flag::center_model)
  {
    center_a3d(&animated_models);
  }

  get_a3d_scale_size(&animated_models);

  get_a3d_header_data(&animated_models);

  if(flags & obj_to_m3d_flag::recalculate_vertex_normals)
  {
    a3d_recalc_vertNorms(&animated_models);
  }



  std::size_t a3d_file_size = get_a3d_file_size(&animated_models);

  // Allocating enough space for *.a3d file.
  m3d_data = std::string(a3d_file_size, '\0');
  m3d_data_cur_pos = 0;

  write_a3d_header_data();
  for(std::size_t i = 0; i < n_models; ++i)
  {
    write_c3d(animated_models[i]);
  }



  boost::filesystem::path file_to_save = output_m3d_path;
  file_to_save.append(model_name + ext::a3d,
                      boost::filesystem::path::codecvt());
  save_file(file_to_save,
            m3d_data,
            file_flag::binary,
            output_file_name_error);
}



void wavefront_obj_to_m3d_model::other_wavefront_objs_to_m3d()
{
  volInt::polyhedron cur_main_model =
    read_obj_prefix(wavefront_obj::prefix::main, c3d::c3d_type::regular);

  volInt::polyhedron cur_main_bound_model;
  volInt::polyhedron *cur_main_bound_model_ptr = nullptr;
  if(!(flags & obj_to_m3d_flag::generate_bound_models))
  {
    cur_main_bound_model =
      read_obj_prefix(wavefront_obj::prefix::main_bound, c3d::c3d_type::bound);
    cur_main_bound_model_ptr = &cur_main_bound_model;
  }

  // Must be called before call to remove_polygons() and get_m3d_header_data().
  read_file_cfg_m3d(cur_main_model);



  // Must be called before call to get_m3d_scale_size().
  remove_polygons(cur_main_model, remove_polygons_model::non_mechos);

  if(flags & obj_to_m3d_flag::center_model)
  {
    center_m3d(&cur_main_model,
               center_m3d_model::non_weapon,
               cur_main_bound_model_ptr);
  }

  get_m3d_scale_size(&cur_main_model, cur_main_bound_model_ptr);

  get_m3d_header_data(&cur_main_model, cur_main_bound_model_ptr);


  if(flags & obj_to_m3d_flag::generate_bound_models)
  {
    m3d_non_mechos_generate_bound(&cur_main_model, cur_main_bound_model);
    cur_main_bound_model_ptr = &cur_main_bound_model;
  }

  if(flags & obj_to_m3d_flag::recalculate_vertex_normals)
  {
    m3d_recalc_vertNorms(&cur_main_model, cur_main_bound_model_ptr);
  }



  std::size_t m3d_file_size =
    get_m3d_file_size(&cur_main_model, cur_main_bound_model_ptr);



  // Allocating enough space for *.m3d file.
  m3d_data = std::string(m3d_file_size, '\0');
  m3d_data_cur_pos = 0;

  write_c3d(cur_main_model);
  write_m3d_header_data();
  write_c3d(*cur_main_bound_model_ptr);
  write_var_to_m3d<int, std::int32_t>(weapon_slots_existence);



  boost::filesystem::path file_to_save = output_m3d_path;
  file_to_save.append(model_name + ext::m3d,
                      boost::filesystem::path::codecvt());
  save_file(file_to_save,
            m3d_data,
            file_flag::binary,
            output_file_name_error);
}



boost::filesystem::path
  wavefront_obj_to_m3d_model::file_prefix_to_path(const std::string &prefix,
                                                  const std::size_t *model_num)
{
  return input_m3d_path / file_prefix_to_filename(prefix, model_num);
}



void wavefront_obj_to_m3d_model::read_file_cfg_helper_overwrite_volume(
  volInt::polyhedron &model,
  const double custom_volume)
{
  model.volume_overwritten = true;
  model.volume = custom_volume;
}



void wavefront_obj_to_m3d_model::read_file_cfg_helper_overwrite_J(
  volInt::polyhedron &model,
  const std::vector<double> &custom_J)
{
  model.J_overwritten = true;
  std::size_t cur_custom_J_el = 0;
  for(std::size_t cur_row = 0; cur_row < volInt::axes_num; ++cur_row)
  {
    for(std::size_t cur_el = 0; cur_el < volInt::axes_num; ++cur_el)
    {
      model.J[cur_row][cur_el] = custom_J[cur_custom_J_el];
      ++cur_custom_J_el;
    }
  }
}



void wavefront_obj_to_m3d_model::read_file_cfg_m3d(
  volInt::polyhedron &main_model,
  std::deque<volInt::polyhedron> *debris_models)
{
  boost::program_options::variables_map vm;

  try
  {
    boost::filesystem::path config_file_path =
      input_m3d_path / (model_name + ".cfg");
    std::string config_file_str = config_file_path.string();

    // Declare a group of options that will be allowed in config file.
    boost::program_options::options_description config("per_file_cfg");
    config.add_options()
      (option::per_file::name::overwrite_volume_main.c_str(),
       boost::program_options::bool_switch()->
         default_value(option::per_file::default_val::overwrite_volume_main),
       "Overwrite volume for main model when custom volume is supplied.")
      (option::per_file::name::custom_center_of_mass_main.c_str(),
       boost::program_options::bool_switch()->default_value(
         option::per_file::default_val::custom_center_of_mass_main),
       "Use center of mass mark to get custom center of mass for main model.")
      (option::per_file::name::overwrite_inertia_tensor_main.c_str(),
       boost::program_options::bool_switch()->default_value(
         option::per_file::default_val::overwrite_inertia_tensor_main),
       "Overwrite inertia tensor matrix for main model "
         "when custom matrix is supplied.")
      (option::per_file::name::custom_volume_main.c_str(),
       boost::program_options::value<std::string>(),
       "Custom volume for main model.")
      (option::per_file::name::custom_inertia_tensor_main.c_str(),
       boost::program_options::value<std::vector<std::string>>(),
       "Custom inertia tensor matrix for main model.")
    ;

    if(debris_models && debris_models->size())
    {
      config.add_options()
        (option::per_file::name::overwrite_volume_debris.c_str(),
         boost::program_options::bool_switch()->default_value(
           option::per_file::default_val::overwrite_volume_debris),
         "Overwrite volume for debris models when custom volume is supplied.")
        (option::per_file::name::custom_center_of_mass_debris.c_str(),
         boost::program_options::bool_switch()->default_value(
           option::per_file::default_val::custom_center_of_mass_debris),
         "Use center of mass mark to get custom center of mass "
           "for debris models.")
        (option::per_file::name::overwrite_inertia_tensor_debris.c_str(),
         boost::program_options::bool_switch()->default_value(
           option::per_file::default_val::overwrite_inertia_tensor_debris),
         "Overwrite inertia tensor matrix for debris models "
           "when custom matrix is supplied.")
      ;
      for(std::size_t cur_debris = 0,
            debris_models_size = debris_models->size();
          cur_debris < debris_models_size;
          ++cur_debris)
      {
        config.add_options()
          ((option::per_file::name::custom_volume_debris + "_" +
              std::to_string(cur_debris + 1)).c_str(),
           boost::program_options::value<std::string>(),
           ("Custom volume for debris model " +
              std::to_string(cur_debris + 1) + ".").c_str())
          ((option::per_file::name::custom_inertia_tensor_debris + "_" +
              std::to_string(cur_debris + 1)).c_str(),
           boost::program_options::value<std::vector<std::string>>(),
           ("Custom inertia tensor matrix for debris model " +
              std::to_string(cur_debris + 1) + ".").c_str())
        ;
      }
    }



    std::ifstream ifs(config_file_str.c_str());
    if(ifs)
    {
      // When debris or animation frame *.obj file is deleted,
      // config option for that debris or animation frame is not expected.
      // To prevent "unrecognised option" error,
      // allow_unregistered argument is set to true for parse_config_file().
      boost::program_options::store(parse_config_file(ifs, config, true), vm);
      boost::program_options::notify(vm);
    }
    else
    {
      throw std::runtime_error(
        "Can not open config file: \"" + config_file_str + "\".");
    }



    if(vm[option::per_file::name::overwrite_volume_main].as<bool>())
    {
      const double custom_volume_main =
        parse_per_file_cfg_option<double>(
          vm[option::per_file::name::custom_volume_main].as<std::string>());
      read_file_cfg_helper_overwrite_volume(main_model, custom_volume_main);
    }

    if(vm[option::per_file::name::custom_center_of_mass_main].as<bool>())
    {
      try
      {
        get_custom_rcm(main_model);
      }
      catch(std::exception &e)
      {
        std::cout << "Failed to retrieve custom center of mass from " <<
          input_file_name_error << " file " <<
          main_model.wavefront_obj_path << '\n';
        std::cout << e.what() << '\n';
        std::cout <<
          "Center of mass is not overwritten for main model." << '\n';
      }
    }

    if(vm[option::per_file::name::overwrite_inertia_tensor_main].as<bool>())
    {
      if(vm.count(option::per_file::name::custom_inertia_tensor_main))
      {
        // 9 doubles to describe 3x3 inertia tensor matrix.
        const std::vector<double> custom_J_main =
          parse_per_file_cfg_multiple_options<double>(
            vm[option::per_file::name::custom_inertia_tensor_main].
              as<std::vector<std::string>>());
        if(custom_J_main.size() == J_cfg_num_of_values)
        {
          read_file_cfg_helper_overwrite_J(main_model, custom_J_main);
        }
        else
        {
          std::cout <<
            "In config file " << config_file_str <<
            " unexpected number of values specified for " <<
            option::per_file::name::custom_inertia_tensor_main << '\n';
          std::cout <<
            "Expected " << std::to_string(J_cfg_num_of_values) <<
            ", got  " << custom_J_main.size() << '\n';
          std::cout <<
            "Inertia tensor is not overwritten for main model." << '\n';
        }
      }
    }


    if(debris_models && debris_models->size())
    {
      if(vm[option::per_file::name::overwrite_volume_debris].as<bool>())
      {
        for(std::size_t cur_debris = 0,
              debris_models_size = debris_models->size();
            cur_debris < debris_models_size;
            ++cur_debris)
        {
          std::string cur_custom_volume_option =
            option::per_file::name::custom_volume_debris + "_" +
            std::to_string(cur_debris + 1);
          if(vm.count(cur_custom_volume_option))
          {
            const double custom_volume_debris =
              parse_per_file_cfg_option<double>(
                vm[cur_custom_volume_option].as<std::string>());
            read_file_cfg_helper_overwrite_volume((*debris_models)[cur_debris],
                                                  custom_volume_debris);
          }
        }
      }

      if(vm[option::per_file::name::custom_center_of_mass_debris].as<bool>())
      {
        for(std::size_t cur_debris = 0,
              debris_models_size = debris_models->size();
            cur_debris < debris_models_size;
            ++cur_debris)
        {
          try
          {
            get_custom_rcm((*debris_models)[cur_debris]);
          }
          catch(std::exception &e)
          {
            std::cout <<
              "Failed to retrieve custom center of mass from " <<
              input_file_name_error << " file " <<
              (*debris_models)[cur_debris].wavefront_obj_path << '\n';
            std::cout << e.what() << '\n';
            std::cout <<
              "Center of mass is not overwritten for debris model " <<
              std::to_string(cur_debris + 1) << "." << '\n';
          }
        }
      }

      if(vm[option::per_file::name::overwrite_inertia_tensor_debris].
           as<bool>())
      {
        for(std::size_t cur_debris = 0,
              debris_models_size = debris_models->size();
            cur_debris < debris_models_size;
            ++cur_debris)
        {
          std::string cur_custom_J_option =
            option::per_file::name::custom_inertia_tensor_debris + "_" +
            std::to_string(cur_debris + 1);
          if(vm.count(cur_custom_J_option))
          {
            // 9 doubles to describe 3x3 inertia tensor matrix.
            const std::vector<double> custom_J_debris =
              parse_per_file_cfg_multiple_options<double>(
                vm[cur_custom_J_option].as<std::vector<std::string>>());
            if(custom_J_debris.size() == J_cfg_num_of_values)
            {
              read_file_cfg_helper_overwrite_J(
                (*debris_models)[cur_debris],
                custom_J_debris);
            }
            else
            {
              std::cout <<
                "In config file " << config_file_str <<
                " unexpected number of values specified " <<
                "for " << cur_custom_J_option << '\n';
              std::cout <<
                "Expected " << std::to_string(J_cfg_num_of_values) <<
                ", got  " << custom_J_debris.size() << '\n';
              std::cout <<
                "Inertia tensor is not overwritten for debris " <<
                std::to_string(cur_debris + 1) << "." << '\n';
            }
          }
        }
      }
    }
  }
  catch(std::exception &)
  {
    std::cout <<
      "Failed to get config options for model " << model_name << '\n';
    throw;
  }
}



void wavefront_obj_to_m3d_model::read_file_cfg_a3d(
  std::deque<volInt::polyhedron> &animated_models)
{
  boost::program_options::variables_map vm;

  try
  {
    boost::filesystem::path config_file_path =
      input_m3d_path / (model_name + ".cfg");
    std::string config_file_str = config_file_path.string();

    // Declare a group of options that will be allowed in config file.
    boost::program_options::options_description config("per_file_cfg");
    config.add_options()
      (option::per_file::name::overwrite_volume_animated.c_str(),
       boost::program_options::bool_switch()->default_value(
         option::per_file::default_val::overwrite_volume_animated),
       "Overwrite volume for animated models when custom volume is supplied.")
      (option::per_file::name::custom_center_of_mass_animated.c_str(),
       boost::program_options::bool_switch()->default_value(
         option::per_file::default_val::custom_center_of_mass_animated),
       "Use center of mass mark to get custom center of mass "
         "for animated models.")
      (option::per_file::name::overwrite_inertia_tensor_animated.c_str(),
       boost::program_options::bool_switch()->default_value(
         option::per_file::default_val::overwrite_inertia_tensor_animated),
       "Overwrite inertia tensor matrix for animated models "
         "when custom matrix is supplied.")
    ;

    for(std::size_t cur_animated = 0,
          animated_models_size = animated_models.size();
        cur_animated < animated_models_size;
        ++cur_animated)
    {
      config.add_options()
        ((option::per_file::name::custom_volume_animated + "_" +
            std::to_string(cur_animated + 1)).c_str(),
         boost::program_options::value<std::string>(),
         ("Custom volume for animated model " +
            std::to_string(cur_animated + 1) + ".").c_str())
        ((option::per_file::name::custom_inertia_tensor_animated + "_" +
            std::to_string(cur_animated + 1)).c_str(),
         boost::program_options::value<std::vector<std::string>>(),
         ("Custom inertia tensor matrix for animated model " +
            std::to_string(cur_animated + 1) + ".").c_str())
      ;
    }



    std::ifstream ifs(config_file_str.c_str());
    if(ifs)
    {
      boost::program_options::store(parse_config_file(ifs, config), vm);
      boost::program_options::notify(vm);
    }
    else
    {
      throw std::runtime_error(
        "Can not open config file: \"" + config_file_str + "\".");
    }



    if(vm[option::per_file::name::overwrite_volume_animated].as<bool>())
    {
      for(std::size_t cur_animated = 0,
            animated_models_size = animated_models.size();
          cur_animated < animated_models_size;
          ++cur_animated)
      {
        std::string cur_custom_volume_option =
          option::per_file::name::custom_volume_animated + "_" +
          std::to_string(cur_animated + 1);
        if(vm.count(cur_custom_volume_option))
        {
          const double custom_volume_animated =
            parse_per_file_cfg_option<double>(
              vm[cur_custom_volume_option].as<std::string>());
          read_file_cfg_helper_overwrite_volume(animated_models[cur_animated],
                                                custom_volume_animated);
        }
      }
    }

    if(vm[option::per_file::name::custom_center_of_mass_animated].as<bool>())
    {
      for(std::size_t cur_animated = 0,
            animated_models_size = animated_models.size();
          cur_animated < animated_models_size;
          ++cur_animated)
      {
        try
        {
          get_custom_rcm(animated_models[cur_animated]);
        }
        catch(std::exception &e)
        {
          std::cout <<
            "Failed to retrieve custom center of mass from " <<
            input_file_name_error << " file " <<
            animated_models[cur_animated].wavefront_obj_path << '\n';
          std::cout << e.what() << '\n';
          std::cout <<
            "Center of mass is not overwritten for animated model " <<
            std::to_string(cur_animated + 1) << "." << '\n';
        }
      }
    }

    if(vm[option::per_file::name::overwrite_inertia_tensor_animated].
         as<bool>())
    {
      for(std::size_t cur_animated = 0,
            animated_models_size = animated_models.size();
          cur_animated < animated_models_size;
          ++cur_animated)
      {
        std::string cur_custom_J_option =
          option::per_file::name::custom_inertia_tensor_animated + "_" +
          std::to_string(cur_animated + 1);
        if(vm.count(cur_custom_J_option))
        {
          // 9 doubles to describe 3x3 inertia tensor matrix.
          const std::vector<double> custom_J_animated =
            parse_per_file_cfg_multiple_options<double>(
              vm[cur_custom_J_option].as<std::vector<std::string>>());
          if(custom_J_animated.size() == J_cfg_num_of_values)
          {
            read_file_cfg_helper_overwrite_J(
              animated_models[cur_animated],
              custom_J_animated);
          }
          else
          {
            std::cout <<
              "In config file " << config_file_str <<
              " unexpected number of values specified " <<
              "for " << cur_custom_J_option << '\n';
            std::cout <<
              "Expected " << std::to_string(J_cfg_num_of_values) <<
              ", got  " << custom_J_animated.size() << '\n';
            std::cout <<
              "Inertia tensor is not overwritten for animated model " <<
              std::to_string(cur_animated + 1) << "." << '\n';
          }
        }
      }
    }
  }
  catch(std::exception &)
  {
    std::cout << "Failed to get config options for model " <<
      model_name << '\n';
    throw;
  }
}



volInt::polyhedron wavefront_obj_to_m3d_model::read_obj(
  const boost::filesystem::path &obj_input_file_path,
  c3d::c3d_type cur_c3d_type)
{
  return raw_obj_to_volInt_model(obj_input_file_path,
                                 input_file_name_error,
                                 cur_c3d_type,
                                 default_c3d_material_id);
}



volInt::polyhedron wavefront_obj_to_m3d_model::read_obj_prefix(
  const std::string &prefix,
  c3d::c3d_type cur_c3d_type)
{
  boost::filesystem::path obj_input_file_path = file_prefix_to_path(prefix);
  return read_obj(obj_input_file_path, cur_c3d_type);
}



std::deque<volInt::polyhedron>
  wavefront_obj_to_m3d_model::read_objs_with_prefix(
    const std::string &prefix,
    c3d::c3d_type cur_c3d_type)
{
  std::deque<volInt::polyhedron> models_to_return;

  for(std::size_t cur_model = 0; ; ++cur_model)
  {
    boost::filesystem::path cur_path = file_prefix_to_path(prefix, &cur_model);
    try
    {
      models_to_return.push_back(read_obj(cur_path, cur_c3d_type));
    }
    // Expected. Stopping the loop when next file in the list is not found.
    catch(exception::file_not_found &)
    {
      break;
    }
  }

  return models_to_return;
}





std::vector<double> wavefront_obj_to_m3d_model::get_medium_vert(
  const volInt::polyhedron &model,
  const volInt::face &poly)
{
  std::vector<double> medium_vert(volInt::axes_num, 0.0);
  // For polygon with zero_reserved color, middle point is different.
  // middle_x is either xmax of *.m3d or -xmax of *.m3d.
  // middle_y is either ymax of *.m3d or -ymax of *.m3d.
  // middle_z for all those polygons is zmin of bound *.c3d.
  // In all other polygons, middle point is average vertex.
  if(poly.color_id ==
     c3d::color::string_to_id::zero_reserved)
  {
    // Preserved sign.
    std::vector<double> extreme_abs_coords(volInt::axes_num, 0.0);
    for(const auto vert_ind : poly.verts)
    {
      const std::vector<double> &vert = model.verts[vert_ind];
      for(std::size_t cur_coord = 0; cur_coord < volInt::axes_num; ++cur_coord)
      {
        if(std::abs(vert[cur_coord]) >
           std::abs(extreme_abs_coords[cur_coord]))
        {
          extreme_abs_coords[cur_coord] = vert[cur_coord];
        }
      }
    }
    if(std::signbit(extreme_abs_coords[0]))
    {
      medium_vert[0] = -xmax();
    }
    else
    {
      medium_vert[0] = xmax();
    }
    if(std::signbit(extreme_abs_coords[1]))
    {
      medium_vert[1] = -ymax();
    }
    else
    {
      medium_vert[1] = ymax();
    }
    medium_vert[2] = model.zmin();
  }
  else
  {
    for(std::size_t vert_n = 0; vert_n < poly.numVerts; ++vert_n)
    {
      volInt::vector_plus_self(medium_vert, model.verts[poly.verts[vert_n]]);
    }
    volInt::vector_divide_self(medium_vert, poly.numVerts);
  }
  return medium_vert;
}


void wavefront_obj_to_m3d_model::write_vertex(const std::vector<double> &vert)
{
  write_vec_var_to_m3d_scaled<double, float>(vert);
  write_vec_var_to_m3d_scaled_rounded<double, char>(vert);
  write_var_to_m3d<int, std::int32_t>(c3d::vertex::default_sort_info);
}

void wavefront_obj_to_m3d_model::write_vertices(
  const volInt::polyhedron &model)
{
  for(std::size_t vert_ind = 0; vert_ind < model.numVerts; ++vert_ind)
  {
    write_vertex(model.verts[vert_ind]);
  }
}


void wavefront_obj_to_m3d_model::write_normal(
  const std::vector<double> &norm,
  bitflag<normal_flag> flags = normal_flag::sort_info)
{
  write_vec_var_to_m3d_rounded<double, char>(
    volInt::vector_scale(c3d::vector_scale_val, norm));
  write_var_to_m3d<unsigned char, unsigned char>(
    c3d::normal::default_n_power);
  if(flags & normal_flag::sort_info)
  {
    write_var_to_m3d<int, std::int32_t>(c3d::normal::default_sort_info);
  }
}

void wavefront_obj_to_m3d_model::write_normals(const volInt::polyhedron &model)
{
  for(std::size_t norm_ind = 0; norm_ind < model.numVertNorms; ++norm_ind)
  {
    write_normal(model.vertNorms[norm_ind]);
  }
}


void wavefront_obj_to_m3d_model::write_polygon(
  const volInt::polyhedron &model,
  const volInt::face &poly)
{
  write_var_to_m3d<int, std::int32_t>(poly.numVerts);
  write_var_to_m3d<int, std::int32_t>(c3d::polygon::default_sort_info);
  write_var_to_m3d<unsigned int, std::uint32_t>(poly.color_id);
  // color_shift is always 0.
  write_var_to_m3d<unsigned int, std::uint32_t>(
    c3d::polygon::default_color_shift);

  write_normal(poly.norm, normal_flag::none);

  std::vector<double> medium_vert = get_medium_vert(model, poly);
  write_vec_var_to_m3d_scaled_rounded<double, char>(medium_vert);

  // Note the reverse order of vertices.
  for(std::size_t vert_f_ind = 0, vert_f_ind_r = poly.numVerts - 1;
      vert_f_ind < poly.numVerts;
      ++vert_f_ind, --vert_f_ind_r)
  {
    write_var_to_m3d<int, std::int32_t>(poly.verts[vert_f_ind_r]);
    write_var_to_m3d<int, std::int32_t>(poly.vertNorms[vert_f_ind_r]);
  }
}

void wavefront_obj_to_m3d_model::write_polygons(
  const volInt::polyhedron &model)
{
  for(std::size_t cur_poly_n = 0; cur_poly_n < model.numFaces; ++cur_poly_n)
  {
    write_polygon(model, model.faces[cur_poly_n]);
  }
}



//// VANGERS SOURCE
//#ifndef COMPACT_3D
//  int poly_ind;
//  for(i = 0; i < 3; i++)
//  {
//    sorted_variable_polygons[i] = HEAP_ALLOC(num_poly, VariablePolygon*);
//    for(j = 0; j < num_poly; j++)
//    {
//      buf > poly_ind;
//      sorted_variable_polygons[i][j] = &variable_polygons[poly_ind];
//    }
//  }
//#else
//  //buf.set(3 * num_poly * sizeof(VariablePolygon*), XB_CUR);
//  buf.set(3 * num_poly * 4, XB_CUR);
//#endif

void wavefront_obj_to_m3d_model::write_sorted_polygon_indices(
  const volInt::polyhedron &model)
{
  std::size_t skipped_sorted_bytes_n =
    model.numFaces * c3d::polygon_sort_info::size;
  std::memset(&m3d_data[m3d_data_cur_pos], 0, skipped_sorted_bytes_n);
  m3d_data_cur_pos += skipped_sorted_bytes_n;
}


void wavefront_obj_to_m3d_model::write_c3d(const volInt::polyhedron &model)
{
  write_var_to_m3d<int, std::int32_t>(c3d::version_req);

  write_var_to_m3d<int, std::int32_t>(model.numVerts);
  write_var_to_m3d<int, std::int32_t>(model.numVertNorms);
  write_var_to_m3d<int, std::int32_t>(model.numFaces);
  write_var_to_m3d<int, std::int32_t>(model.numVertTotal);

  write_vec_var_to_m3d_scaled_rounded<double, std::int32_t>(model.max_point());
  write_vec_var_to_m3d_scaled_rounded<double, std::int32_t>(model.min_point());
  write_vec_var_to_m3d_scaled_rounded<double, std::int32_t>(
    model.offset_point());
  write_var_to_m3d_scaled_rounded<double, std::int32_t>(model.rmax);

  write_vec_var_to_m3d<int, std::int32_t>(c3d::default_phi_psi_tetta);

  write_var_to_m3d_scaled<double, double>(model.volume, 3.0);
  write_vec_var_to_m3d_scaled<double, double>(model.rcm);
  write_nest_vec_var_to_m3d_scaled<double, double>(model.J, 5.0);

  write_vertices(model);
  write_normals(model);
  write_polygons(model);

  write_sorted_polygon_indices(model);
}



void wavefront_obj_to_m3d_model::write_m3d_header_data()
{
  write_vec_var_to_m3d_scaled_rounded<double, std::int32_t>(max_point());
  write_var_to_m3d_scaled_rounded<double, std::int32_t>(rmax);
  write_var_to_m3d<int, std::int32_t>(n_wheels);
  write_var_to_m3d<int, std::int32_t>(n_debris);
  write_var_to_m3d<int, std::int32_t>(body_color_offset);
  write_var_to_m3d<int, std::int32_t>(body_color_shift);
}



void wavefront_obj_to_m3d_model::write_a3d_header_data()
{
  write_var_to_m3d<int, std::int32_t>(n_models);
  write_vec_var_to_m3d_scaled_rounded<double, std::int32_t>(max_point());
  write_var_to_m3d_scaled_rounded<double, std::int32_t>(rmax);
  write_var_to_m3d<int, std::int32_t>(body_color_offset);
  write_var_to_m3d<int, std::int32_t>(body_color_shift);
}



void wavefront_obj_to_m3d_model::write_m3d_wheel_data(
  std::unordered_map<int, volInt::polyhedron> &wheels_models,
  std::size_t wheel_id)
{
  write_var_to_m3d<int, std::int32_t>(cur_wheel_data[wheel_id].steer);
  write_vec_var_to_m3d_scaled<double, double>(cur_wheel_data[wheel_id].r);
  write_var_to_m3d_scaled_rounded<double, std::int32_t>(
    cur_wheel_data[wheel_id].width);
  write_var_to_m3d_scaled_rounded<double, std::int32_t>(
    cur_wheel_data[wheel_id].radius);
  write_var_to_m3d<int, std::int32_t>(m3d::wheel::default_bound_index);
  if(cur_wheel_data[wheel_id].steer)
  {
    write_c3d(wheels_models[wheel_id]);
  }
}

void wavefront_obj_to_m3d_model::write_m3d_wheels_data(
  std::unordered_map<int, volInt::polyhedron> &wheels_models)
{
  for(std::size_t wheel_id = 0; wheel_id < n_wheels; ++wheel_id)
  {
    write_m3d_wheel_data(wheels_models, wheel_id);
  }
}



void wavefront_obj_to_m3d_model::write_m3d_debris_data(
  volInt::polyhedron &debris_model,
  volInt::polyhedron &debris_bound_model)
{
  write_c3d(debris_model);
  write_c3d(debris_bound_model);
}

void wavefront_obj_to_m3d_model::write_m3d_debris_data(
  std::deque<volInt::polyhedron> &debris_models,
  std::deque<volInt::polyhedron> &debris_bound_models)
{
  for(std::size_t debris_num = 0; debris_num < n_debris; ++debris_num)
  {
    write_m3d_debris_data(debris_models[debris_num],
                          debris_bound_models[debris_num]);
  }
}



void wavefront_obj_to_m3d_model::write_m3d_weapon_slot(std::size_t slot_id)
{
  write_vec_var_to_m3d_scaled_rounded<double, std::int32_t>(
    cur_weapon_slot_data[slot_id].R_slot);
  write_var_to_m3d<int, std::int32_t>(
    volInt::radians_to_sicher_angle(
      cur_weapon_slot_data[slot_id].location_angle_of_slot));
}

void wavefront_obj_to_m3d_model::write_m3d_weapon_slots()
{
  for(std::size_t slot_id = 0;
      slot_id < m3d::weapon_slot::max_slots;
      ++slot_id)
  {
    write_m3d_weapon_slot(slot_id);
  }
}





void wavefront_obj_to_m3d_model::for_each_steer_non_ghost_wheel(
  const volInt::polyhedron *main_model,
  const std::unordered_map<int, volInt::polyhedron> *wheels_models,
  std::function<void(const volInt::polyhedron&)> func_to_call)
{
  if(wheels_models)
  {
    for(const auto wheel_steer_num : main_model->wheels_steer)
    {
      if(main_model->wheels_non_ghost.count(wheel_steer_num))
      {
        func_to_call((*wheels_models).at(wheel_steer_num));
      }
    }
  }
}

void wavefront_obj_to_m3d_model::for_each_steer_non_ghost_wheel(
  const volInt::polyhedron *main_model,
  std::unordered_map<int, volInt::polyhedron> *wheels_models,
  std::function<void(volInt::polyhedron&)> func_to_call)
{
  if(wheels_models)
  {
    for(const auto wheel_steer_num : main_model->wheels_steer)
    {
      if(main_model->wheels_non_ghost.count(wheel_steer_num))
      {
        func_to_call((*wheels_models).at(wheel_steer_num));
      }
    }
  }
}





std::vector<point*>
  wavefront_obj_to_m3d_model::get_ref_points_for_part_of_model(
    volInt::polyhedron &model,
    const volInt::polyhedron *reference_model,
    unsigned int color_id,
    int wheel_id,
    int weapon_id)
{
  std::vector<point*> ref_points(3, nullptr);

  std::pair<int, int> vert_indices(0, 0);

  for(std::size_t poly_ind = 0; poly_ind < model.numFaces; ++poly_ind)
  {
    volInt::face &cur_poly = model.faces[poly_ind];
    if(cur_poly.color_id == color_id &&
       (cur_poly.wheel_id == volInt::invalid::wheel_id ||
        cur_poly.wheel_id == wheel_id) &&
       (cur_poly.weapon_id == volInt::invalid::weapon_id ||
        cur_poly.weapon_id == weapon_id))
    {
      for(std::size_t v_f_ind = 0; v_f_ind < model.numVertsPerPoly; ++v_f_ind)
      {
        // ref_vert_one_ind is std::pair<int, int>.
        // First is position of polygon which contains vertex of reference.
        // Second is vertex index in that polygon.
        if(reference_model->ref_vert_one_ind == vert_indices)
        {
          ref_points[0] = &model.verts[model.faces[poly_ind].verts[v_f_ind]];
        }
        else if(reference_model->ref_vert_two_ind == vert_indices)
        {
          ref_points[1] = &model.verts[model.faces[poly_ind].verts[v_f_ind]];
        }
        else if(reference_model->ref_vert_three_ind == vert_indices)
        {
          ref_points[2] = &model.verts[model.faces[poly_ind].verts[v_f_ind]];
        }
        ++vert_indices.second;
      }
      ++vert_indices.first;
      vert_indices.second = 0;
    }
  }

  if(!(ref_points[0] &&
       ref_points[1] &&
       ref_points[2]))
  {
    std::string err_msg;
    err_msg.append(
      input_file_name_error + " file " + model.wavefront_obj_path + "\n" +
      "Couldn't find reference points for " +
      c3d::color::ids.by<c3d::color::id>().at(color_id));
    if(wheel_id != volInt::invalid::wheel_id)
    {
      err_msg.append(", wheel_id: " + std::to_string(wheel_id + 1));
    }
    if(weapon_id != volInt::invalid::weapon_id)
    {
      err_msg.append(", weapon_id: " + std::to_string(weapon_id + 1));
    }
    err_msg.append(".\n");
    throw exception::model_ref_points_not_found(err_msg);
  }

  return ref_points;
}



std::pair<point, point> wavefront_obj_to_m3d_model::get_compare_points(
  std::vector<point*> cur_ref_verts,
  const volInt::polyhedron *reference_model)
{

  point two_rel_to_one = volInt::vector_minus((*cur_ref_verts[1]),
                                              (*cur_ref_verts[0]));

  point three_rel_to_one = volInt::vector_minus((*cur_ref_verts[2]),
                                                (*cur_ref_verts[0]));

  point two_compare_point =
    volInt::vector_minus(two_rel_to_one,
                         reference_model->ref_vert_two_rel_to_one);
  point three_compare_point =
    volInt::vector_minus(three_rel_to_one,
                         reference_model->ref_vert_three_rel_to_one);

  return {two_compare_point, three_compare_point};
}



std::pair<point, point> wavefront_obj_to_m3d_model::get_compare_points(
  std::vector<point*> cur_ref_verts,
  std::vector<point*> ref_model_ref_verts)
{

  point two_rel_to_one =
    volInt::vector_minus((*cur_ref_verts[1]), (*cur_ref_verts[0]));

  point three_rel_to_one =
    volInt::vector_minus((*cur_ref_verts[2]), (*cur_ref_verts[0]));

  point ref_model_two_rel_to_one =
    volInt::vector_minus((*ref_model_ref_verts[1]), (*ref_model_ref_verts[0]));

  point ref_model_three_rel_to_one =
    volInt::vector_minus((*ref_model_ref_verts[2]), (*ref_model_ref_verts[0]));

  point two_compare_point =
    volInt::vector_minus(two_rel_to_one, ref_model_two_rel_to_one);
  point three_compare_point =
    volInt::vector_minus(three_rel_to_one, ref_model_three_rel_to_one);

  return {two_compare_point, three_compare_point};
}



// rcm - center of mass.
void wavefront_obj_to_m3d_model::get_custom_rcm(volInt::polyhedron &model)
{
  if(!center_of_mass_model)
  {
    return;
  }

  std::vector<point*> cur_rcm_verts =
    get_ref_points_for_part_of_model(
      model,
      center_of_mass_model,
      c3d::color::string_to_id::center_of_mass);

  std::pair<point, point> compare_points =
    get_compare_points(
      cur_rcm_verts,
      center_of_mass_model);



  // If all coordinates of points 2 and 3 relative to 1 are the same
  // in orig model and current model, then model wasn't rotated or changed.
  // compare_points.first is difference between 2nd reference points.
  // compare_points.second is difference between 3rd ones.
  // Throwing exception if difference is not zero.
  for(std::size_t cur_coord = 0; cur_coord < volInt::axes_num; ++cur_coord)
  {
    if(!(std::abs(compare_points.first[cur_coord]) <
         volInt::distinct_distance &&
       std::abs(compare_points.second[cur_coord]) <
         volInt::distinct_distance))
    {
      throw std::runtime_error(
        input_file_name_error + " file " +
        model.wavefront_obj_path + '\n' +
        "Center of mass was rotated, scaled or changed." + '\n' +
        "Only moving is allowed for center of mass." + '\n');
    }
  }

  model.rcm = volInt::vector_minus(*cur_rcm_verts[0],
                                   *center_of_mass_model->ref_vert_one);

  model.rcm_overwritten = true;
}



void wavefront_obj_to_m3d_model::get_attachment_point(
  volInt::polyhedron &model)
{
  if(!weapon_attachment_point)
  {
    return;
  }

  std::vector<point*> cur_attachment_verts =
    get_ref_points_for_part_of_model(
      model,
      weapon_attachment_point,
      c3d::color::string_to_id::attachment_point);

  std::pair<point, point> compare_points =
    get_compare_points(
      cur_attachment_verts,
      weapon_attachment_point);



  // If all coordinates of points 2 and 3 relative to 1 are the same
  // in orig model and current model, then model wasn't rotated or changed.
  // compare_points.first is difference between 2nd reference points.
  // compare_points.second is difference between 3rd ones.
  // Throwing exception if difference is not zero.
  for(std::size_t cur_coord = 0; cur_coord < volInt::axes_num; ++cur_coord)
  {
    if(!(std::abs(compare_points.first[cur_coord]) <
         volInt::distinct_distance &&
       std::abs(compare_points.second[cur_coord]) <
         volInt::distinct_distance))
    {
      throw std::runtime_error(
        input_file_name_error + " file " +
        model.wavefront_obj_path + '\n' +
        "Attachment point was rotated, scaled or changed." + '\n' +
        "Only moving is allowed for attachment point in weapon model." + '\n');
    }
  }

  model.offset_point() =
    volInt::vector_minus(*cur_attachment_verts[0],
                         *weapon_attachment_point->ref_vert_one);
}



// Using weapon attachment point model to get position of weapons.
void wavefront_obj_to_m3d_model::get_weapons_data(volInt::polyhedron &model)
{
  weapon_slots_existence = 0;
  cur_weapon_slot_data =
    std::vector<weapon_slot_data>(m3d::weapon_slot::max_slots);

  if(!weapon_attachment_point)
  {
    return;
  }

  // There are m3d::weapon_slot::max_slots weapons in all Vangers models.
  // cur_weapon_verts contains pair of vertices for each weapon.
  // That pair of vertices is later used to get weapons' angles.
  std::vector<std::vector<point*>> cur_weapon_verts;
  cur_weapon_verts.reserve(m3d::weapon_slot::max_slots);



  for(std::size_t cur_slot = 0;
      cur_slot < m3d::weapon_slot::max_slots;
      ++cur_slot)
  {
    try
    {
      cur_weapon_verts.push_back(
        get_ref_points_for_part_of_model(
          model,
          weapon_attachment_point,
          c3d::color::string_to_id::attachment_point,
          volInt::invalid::wheel_id,
          cur_slot));
    }
    // If get_ref_points_for_part_of_model failed to find reference points.
    catch(exception::model_ref_points_not_found &)
    {
      cur_weapon_verts.push_back(std::vector<point*>(3, nullptr));
    }
  }



  // Getting info about slots.
  for(std::size_t cur_slot = 0;
      cur_slot < m3d::weapon_slot::max_slots;
      ++cur_slot)
  {
    if(cur_weapon_verts[cur_slot][0] &&
       cur_weapon_verts[cur_slot][1] &&
       cur_weapon_verts[cur_slot][2])
    {
      std::pair<point, point> compare_points =
        get_compare_points(
          cur_weapon_verts[cur_slot],
          weapon_attachment_point);

      weapon_slots_existence |= (1 << cur_slot);
      cur_weapon_slot_data[cur_slot].exists = true;

      point two_rel_to_one =
        volInt::vector_minus((*cur_weapon_verts[cur_slot][1]),
                             (*cur_weapon_verts[cur_slot][0]));

      // If y coordinate of points 2 and 3 relative to 1 is the same
      // in orig model and current model,
      // then model wasn't rotated by x or z axes.
      // compare_points.first is difference between 2nd reference points.
      // compare_points.second is difference between 3rd ones.
      // Throwing exception if difference of y coordinate is not zero.
      if(!(std::abs(compare_points.first[1]) <
             volInt::distinct_distance &&
           std::abs(compare_points.second[1]) <
             volInt::distinct_distance))
      {
        throw std::runtime_error(
          input_file_name_error + " file " +
          model.wavefront_obj_path + "\n" +
          "Weapon " + std::to_string(cur_slot + 1) +
          " was rotated by x or z axis, scaled or changed." + "\n" +
          "Only moving or rotating by y axis is allowed " +
          "for attachment point in mechos model." + '\n');
      }

      cur_weapon_slot_data[cur_slot].location_angle_of_slot =
        std::remainder(
          std::atan2(two_rel_to_one[0], two_rel_to_one[2]) -
            weapon_attachment_point->ref_angle,
          2 * M_PI);



      std::vector<point> ref_verts_rotated
      {
        *weapon_attachment_point->ref_vert_one,
        *weapon_attachment_point->ref_vert_two,
        *weapon_attachment_point->ref_vert_three,
      };
      for(auto &&ref_vert : ref_verts_rotated)
      {
        volInt::rotate_point_by_axis(
          ref_vert,
          cur_weapon_slot_data[cur_slot].location_angle_of_slot,
          volInt::rotation_axis::y);
      }

      std::vector<point*> ref_verts_rotated_ptr
      {
        &ref_verts_rotated[0],
        &ref_verts_rotated[1],
        &ref_verts_rotated[2],
      };

      std::pair<point, point> rotated_compare_points =
        get_compare_points(
          cur_weapon_verts[cur_slot],
          ref_verts_rotated_ptr);

      // If all coordinates of points 2 and 3 relative to 1 are the same
      // in rotated orig model and current model, then model wasn't changed.
      // rotated_compare_points.first is difference
      // between 2nd reference points.
      // rotated_compare_points.second is difference between 3rd ones.
      // Throwing exception if difference is not zero.
      for(std::size_t coord_el = 0 ; coord_el < volInt::axes_num; ++coord_el)
      {
        if(!(std::abs(rotated_compare_points.first[coord_el]) <
               volInt::distinct_distance &&
             std::abs(rotated_compare_points.second[coord_el]) <
               volInt::distinct_distance))
        {
          throw std::runtime_error(
            input_file_name_error + " file " +
            model.wavefront_obj_path + "\n" +
            "Weapon " + std::to_string(cur_slot + 1) +
            " was scaled or changed." + "\n" +
            "Only moving or rotating by y axis is allowed " +
            "for attachment point in mechos model." + '\n');
        }
      }

      cur_weapon_slot_data[cur_slot].R_slot =
        volInt::vector_minus((*cur_weapon_verts[cur_slot][0]),
                             ref_verts_rotated[0]);
    }
  }
}





std::unordered_map<int, volInt::polyhedron>
  wavefront_obj_to_m3d_model::get_wheels_steer(
    volInt::polyhedron &main_model)
{
  std::unordered_map<int, volInt::polyhedron> wheels_steer_models;
  std::unordered_map<int, std::size_t> cur_vert_nums;
  std::unordered_map<int, std::size_t> cur_norm_nums;
  std::unordered_map<int, std::size_t> cur_poly_nums;
  std::unordered_map<int, std::unordered_map<int, int>> vertices_maps;
  std::unordered_map<int, std::unordered_map<int, int>> norms_maps;
  std::size_t v_per_poly = main_model.faces[0].numVerts;

  for(const auto wheel_steer_num : main_model.wheels_steer)
  {
    wheels_steer_models.emplace(wheel_steer_num, volInt::polyhedron());

    if(main_model.wheels_non_ghost.count(wheel_steer_num))
    {
      volInt::polyhedron &cur_wheel_model =
        wheels_steer_models[wheel_steer_num];

      cur_wheel_model.faces.reserve(main_model.numFaces);

      cur_vert_nums[wheel_steer_num] = 0;
      cur_norm_nums[wheel_steer_num] = 0;
      cur_poly_nums[wheel_steer_num] = 0;
    }
  }

  // Copying right polygons to wheel models and making verts and norms maps.
  for(std::size_t poly_ind = 0; poly_ind < main_model.numFaces; ++poly_ind)
  {
    volInt::face &cur_poly = main_model.faces[poly_ind];
    if(main_model.wheels_steer.count(cur_poly.wheel_id) &&
       main_model.wheels_non_ghost.count(cur_poly.wheel_id))
    {
      volInt::polyhedron &cur_wheel_model =
        wheels_steer_models[cur_poly.wheel_id];
      int wheel_id = cur_poly.wheel_id;

      cur_wheel_model.faces.push_back(cur_poly);
      volInt::face &cur_wheel_poly =
        cur_wheel_model.faces[cur_poly_nums[wheel_id]];

      // Checking whether vertices' and normals' indices
      // are already in the maps and adding them if they are not.
      // Key of the map is original vertex/normal index.
      // Value of the map is new vertex/normal index.
      for(std::size_t v_f_ind = 0; v_f_ind < v_per_poly; ++v_f_ind)
      {
        if(vertices_maps[wheel_id].count(cur_wheel_poly.verts[v_f_ind]))
        {
          // One of the keys of the map is equal to vertex index.
          // Changing index of vertex to value of the map entry.
          cur_wheel_poly.verts[v_f_ind] =
            vertices_maps[wheel_id][cur_wheel_poly.verts[v_f_ind]];
        }
        else
        {
          // Vertex index is not found as key in the map.
          // Inserting new key-value pair in the map.
          // Changing index of vertex to size of the map.
          cur_wheel_poly.verts[v_f_ind] = cur_vert_nums[wheel_id];
          vertices_maps[wheel_id][cur_poly.verts[v_f_ind]] =
            cur_vert_nums[wheel_id];
          ++cur_vert_nums[wheel_id];
        }

        if(norms_maps[wheel_id].count(cur_wheel_poly.vertNorms[v_f_ind]))
        {
          // One of the keys of the map is equal to normal index.
          // Changing index of normal to value of the map entry.
          cur_wheel_poly.vertNorms[v_f_ind] =
            norms_maps[wheel_id][cur_wheel_poly.vertNorms[v_f_ind]];
        }
        else
        {
          // Normal index is not found as key in the map.
          // Inserting new key-value pair in the map.
          // Changing index of normal to size of the map.
          cur_wheel_poly.vertNorms[v_f_ind] = cur_norm_nums[wheel_id];
          norms_maps[wheel_id][cur_poly.vertNorms[v_f_ind]] =
            cur_norm_nums[wheel_id];
          ++cur_norm_nums[wheel_id];
        }
      }
      ++cur_poly_nums[wheel_id];
    }
  }

  // Copying vertices and normals into wheel models.
  // Maps created earlier are used to figure out where to copy each vertex.
  for(const auto wheel_steer_num : main_model.wheels_steer)
  {
    if(main_model.wheels_non_ghost.count(wheel_steer_num))
    {
      volInt::polyhedron &cur_wheel_model =
        wheels_steer_models[wheel_steer_num];

      cur_wheel_model.numVerts = vertices_maps[wheel_steer_num].size();
      cur_wheel_model.numVertNorms = norms_maps[wheel_steer_num].size();
      cur_wheel_model.numFaces = cur_wheel_model.faces.size();
      cur_wheel_model.numVertTotal = cur_wheel_model.numFaces * v_per_poly;
      cur_wheel_model.numVertsPerPoly = v_per_poly;

      cur_wheel_model.verts =
        std::vector<std::vector<double>>(cur_wheel_model.numVerts);
      cur_wheel_model.vertNorms =
        std::vector<std::vector<double>>(cur_wheel_model.numVertNorms);

      cur_wheel_model.wavefront_obj_path = main_model.wavefront_obj_path;
      cur_wheel_model.wheel_id = wheel_steer_num;

      // Copying vertices and normals.
      for(const auto &cur_wheel_vertex_ind : vertices_maps[wheel_steer_num])
      {
        cur_wheel_model.verts[cur_wheel_vertex_ind.second] =
          main_model.verts[cur_wheel_vertex_ind.first];
      }
      for(const auto &cur_wheel_norm_ind : norms_maps[wheel_steer_num])
      {
        cur_wheel_model.vertNorms[cur_wheel_norm_ind.second] =
          main_model.vertNorms[cur_wheel_norm_ind.first];
      }

      // Moving coordinate system to wheel center,
      // so center of wheel model will have coordinates: 0, 0, 0.
      point wheel_center = cur_wheel_model.get_model_center();
      cur_wheel_model.offset_point() = wheel_center;
      cur_wheel_model.move_coord_system_to_point_inv_neg_vol(wheel_center);
    }
  }


  return wheels_steer_models;
}



void wavefront_obj_to_m3d_model::get_m3d_extreme_points(
  const volInt::polyhedron *main_model,
  const std::unordered_map<int, volInt::polyhedron> *wheels_models)
{
  extreme_points = volInt::model_extreme_points();
  extreme_points.get_most_extreme_cmp_cur(main_model->extreme_points);

  for_each_steer_non_ghost_wheel(
    main_model, wheels_models,
    [&](const volInt::polyhedron &wheel_model)
      {
        volInt::model_extreme_points cur_wheel_extreme_points =
          wheel_model.extreme_points;
        volInt::vector_plus_self(cur_wheel_extreme_points.max(),
                                 wheel_model.offset_point());
        volInt::vector_plus_self(cur_wheel_extreme_points.min(),
                                 wheel_model.offset_point());
        extreme_points.get_most_extreme_cmp_cur(cur_wheel_extreme_points);
      });
}

void wavefront_obj_to_m3d_model::get_m3d_extreme_points_calc_c3d_extr(
  volInt::polyhedron *main_model,
  std::unordered_map<int, volInt::polyhedron> *wheels_models)
{
  main_model->get_extreme_points();
  for_each_steer_non_ghost_wheel(
    main_model, wheels_models,
    [](volInt::polyhedron &wheel_model)
      {
        wheel_model.get_extreme_points();
      });
  get_m3d_extreme_points(main_model, wheels_models);
}



void wavefront_obj_to_m3d_model::get_a3d_extreme_points(
  const std::deque<volInt::polyhedron> *models)
{
  extreme_points = volInt::model_extreme_points();
  for(const auto &model : *models)
  {
    extreme_points.get_most_extreme_cmp_cur(model.extreme_points);
  }
}

void wavefront_obj_to_m3d_model::get_a3d_extreme_points_calc_c3d_extr(
  std::deque<volInt::polyhedron> *models)
{
  for(auto &&model : *models)
  {
    model.get_extreme_points();
  }
  get_a3d_extreme_points(models);
}



void wavefront_obj_to_m3d_model::get_m3d_header_data(
  volInt::polyhedron *main_model,
  volInt::polyhedron *main_bound_model,
  std::unordered_map<int, volInt::polyhedron> *wheels_models,
  std::deque<volInt::polyhedron> *debris_models,
  std::deque<volInt::polyhedron> *debris_bound_models)
{
  main_model->calculate_c3d_properties();
  if(main_bound_model)
  {
    main_bound_model->calculate_c3d_properties();
  }

  for_each_steer_non_ghost_wheel(
    main_model, wheels_models,
    [](volInt::polyhedron &wheel_model)
      {
        wheel_model.calculate_c3d_properties();
      });

  if(debris_models)
  {
    for(auto &&model : *debris_models)
    {
      model.calculate_c3d_properties();
    }
  }
  if(debris_bound_models)
  {
    for(auto &&model : *debris_bound_models)
    {
      model.calculate_c3d_properties();
    }
  }

  get_m3d_extreme_points(main_model, wheels_models);
  // rmax must be set in get_m3d_scale_size() function.

  body_color_offset = main_model->bodyColorOffset;
  body_color_shift = main_model->bodyColorShift;
}



void wavefront_obj_to_m3d_model::get_a3d_header_data(
  std::deque<volInt::polyhedron> *models)
{
  n_models = models->size();
  if(!n_models)
  {
    boost::filesystem::path first_model_path =
      (input_m3d_path / (model_name + "_1" + ext::obj));
    throw std::runtime_error(
      input_file_name_error + " file " +
      first_model_path.string() + " is not valid " + ext::readable::obj +
      " model." + '\n' +
      "Expecting first model of " + ext::readable::a3d +
      " to be valid." + '\n');
  }

  for(auto &&model : *models)
  {
    model.calculate_c3d_properties();
  }

  get_a3d_extreme_points(models);
  // rmax must be set in get_a3d_scale_size() function.

  body_color_offset = (*models)[0].bodyColorOffset;
  body_color_shift =  (*models)[0].bodyColorShift;
}



void wavefront_obj_to_m3d_model::get_wheels_data(
  const volInt::polyhedron &main_model)
{
  n_wheels = main_model.wheels.size();

  cur_wheel_data = std::vector<wheel_data>(n_wheels);

  // Getting extreme points of wheels to get wheel data.
  std::unordered_map<int, volInt::model_extreme_points> wheels_extreme_points;
  for(std::size_t wheel_n = 0; wheel_n < n_wheels; ++wheel_n)
  {
    wheels_extreme_points[wheel_n] = volInt::model_extreme_points();
  }

  for(const auto &cur_poly : main_model.faces)
  {
    if(main_model.wheels.count(cur_poly.wheel_id))
    {
      for(const auto cur_vert_ind : cur_poly.verts)
      {
        wheels_extreme_points[cur_poly.wheel_id].
          get_most_extreme_cmp_cur(main_model.verts[cur_vert_ind]);
      }
    }
  }

  // Getting wheel data using extreme coords.
  for(std::size_t wheel_n = 0; wheel_n < n_wheels; ++wheel_n)
  {
    cur_wheel_data[wheel_n].wheel_model_index = wheel_n;
    if(main_model.wheels_steer.count(wheel_n))
    {
      cur_wheel_data[wheel_n].steer = 1;
    }
    else
    {
      cur_wheel_data[wheel_n].steer = 0;
    }
    if(main_model.wheels_ghost.count(wheel_n))
    {
      cur_wheel_data[wheel_n].ghost = 1;
    }
    else
    {
      cur_wheel_data[wheel_n].ghost = 0;
    }

    cur_wheel_data[wheel_n].r = wheels_extreme_points[wheel_n].get_center();
    cur_wheel_data[wheel_n].width =
      wheels_extreme_points[wheel_n].xmax() -
      wheels_extreme_points[wheel_n].xmin();
    cur_wheel_data[wheel_n].radius =
      (wheels_extreme_points[wheel_n].zmax() -
       wheels_extreme_points[wheel_n].zmin()) / 2;
  }
}



void wavefront_obj_to_m3d_model::get_debris_data(
  const std::deque<volInt::polyhedron> *debris_models,
  const std::deque<volInt::polyhedron> *debris_bound_models)
{
  std::size_t debris_num = debris_models->size();
  if(debris_bound_models)
  {
    std::size_t debris_bound_models_size = debris_bound_models->size();
    if(debris_num > debris_bound_models_size)
    {
      debris_num = debris_bound_models_size;
    }
  }
  n_debris = debris_num;

  // Whole point of informing about found debris is to make sure
  // that both debris model and debris bound model are found.
  if(!(flags & obj_to_m3d_flag::generate_bound_models))
  {
    if(n_debris)
    {
      std::cout << '\n';
      std::cout << "Found " << std::to_string(debris_num) <<
        " debris for " << input_m3d_path << " model." << '\n';
    }
    else
    {
      std::cout << '\n';
      std::cout << "Couldn't find debris for " <<
        input_m3d_path << " model." << '\n';
    }
  }
}





void wavefront_obj_to_m3d_model::center_debris(
  volInt::polyhedron *debris_model,
  volInt::polyhedron *debris_bound_model)
{
  debris_model->offset_point() = debris_model->get_model_center();
  debris_model->move_coord_system_to_point_inv_neg_vol(
    debris_model->offset_point());
  if(debris_model->rcm_overwritten)
  {
    volInt::vector_minus_self(debris_model->rcm, debris_model->offset_point());
  }
  if(debris_bound_model)
  {
    // Not needed since offset of debris bound model is never used.
    // Added for consistency.
    debris_bound_model->offset_point() = debris_model->offset_point();
    debris_bound_model->move_coord_system_to_point_inv_neg_vol(
      debris_model->offset_point());
  }
}

void wavefront_obj_to_m3d_model::center_debris(
  std::deque<volInt::polyhedron> *debris_models,
  std::deque<volInt::polyhedron> *debris_bound_models)
{
  if(debris_bound_models)
  {
    for(std::size_t i = 0; i < n_debris; ++i)
    {
      center_debris(&(*debris_models)[i], &(*debris_bound_models)[i]);
    }
  }
  else
  {
    for(std::size_t i = 0; i < n_debris; ++i)
    {
      center_debris(&(*debris_models)[i]);
    }
  }
}





void wavefront_obj_to_m3d_model::center_m3d(
  volInt::polyhedron *main_model,
  center_m3d_model model_type,
  volInt::polyhedron *main_bound_model,
  std::unordered_map<int, volInt::polyhedron> *wheels_models,
  std::deque<volInt::polyhedron> *debris_models,
  std::deque<volInt::polyhedron> *debris_bound_models)
{
  get_m3d_extreme_points_calc_c3d_extr(main_model, wheels_models);
  std::vector<double> m3d_center = extreme_points.get_center();

  main_model->move_coord_system_to_point_inv_neg_vol(m3d_center);
  if(model_type == center_m3d_model::weapon)
  {
    volInt::vector_minus_self(main_model->offset_point(), m3d_center);
  }
  if(main_model->rcm_overwritten)
  {
    volInt::vector_minus_self(main_model->rcm, m3d_center);
  }

  if(main_bound_model)
  {
    main_bound_model->move_coord_system_to_point_inv_neg_vol(m3d_center);
  }

  for(std::size_t wheel_n = 0; wheel_n < n_wheels; ++wheel_n)
  {
    volInt::vector_minus_self(cur_wheel_data[wheel_n].r, m3d_center);
  }
  for_each_steer_non_ghost_wheel(
    main_model, wheels_models,
    [&](volInt::polyhedron &wheel_model)
      {
        volInt::vector_minus_self(wheel_model.offset_point(), m3d_center);
      });

  if(debris_models)
  {
    for(auto &&model : *debris_models)
    {
      volInt::vector_minus_self(model.offset_point(), m3d_center);
    }
  }
  if(debris_bound_models)
  {
    for(auto &&model : *debris_bound_models)
    {
      volInt::vector_minus_self(model.offset_point(), m3d_center);
    }
  }
  if(weapon_slots_existence)
  {
    for(std::size_t slot_id = 0;
        slot_id < m3d::weapon_slot::max_slots;
        ++slot_id)
    {
      weapon_slot_data &cur_slot = cur_weapon_slot_data[slot_id];
      if(cur_slot.exists)
      {
        volInt::vector_minus_self(cur_slot.R_slot, m3d_center);
      }
    }
  }
}



void wavefront_obj_to_m3d_model::center_a3d(
  std::deque<volInt::polyhedron> *models)
{
  get_a3d_extreme_points_calc_c3d_extr(models);
  std::vector<double> a3d_center = extreme_points.get_center();

  for(auto &&model : *models)
  {
    model.move_coord_system_to_point_inv_neg_vol(a3d_center);
  }
}





void wavefront_obj_to_m3d_model::get_scale_helper_get_extreme_radius(
  volInt::polyhedron *model,
  double &extreme_radius,
  const point offset)
{
  model->calculate_rmax();
  get_extreme_radius(extreme_radius, model->rmax, offset);
}



void wavefront_obj_to_m3d_model::get_scale_helper_set_scale_from_rmax()
{
  scale_size = c3d::scaling_max_extreme_radius / rmax;
  double prm_lst_scale_size = 1 / scale_size;
  if(prm_lst_scale_size - scale_cap > volInt::distinct_distance)
  {
    std::cout << '\n';
    std::cout << option::name::scale_cap << " " << scale_cap <<
      " is lower than calculated scale_size " << prm_lst_scale_size <<
      " of " << input_m3d_path << " model." << '\n';
    std::cout << option::name::scale_cap << " is written to ";
    if(non_mechos_scale_sizes)
    {
      std::cout << file::game_lst;
    }
    else
    {
      std::cout << model_name << ext::prm;
    }
    std::cout << " file instead of calculated scale_size." << '\n';
    prm_lst_scale_size = scale_cap;
  }

  if(non_mechos_scale_sizes)
  {
    (*non_mechos_scale_sizes)[model_name] = prm_lst_scale_size;
  }
  else
  {
    prm_scale_size = prm_lst_scale_size;
  }
}



void wavefront_obj_to_m3d_model::get_m3d_scale_size(
  volInt::polyhedron *main_model,
  volInt::polyhedron *main_bound_model,
  std::unordered_map<int, volInt::polyhedron> *wheels_models,
  std::deque<volInt::polyhedron> *debris_models,
  std::deque<volInt::polyhedron> *debris_bound_models)
{
  double extreme_radius = 0.0;

  get_scale_helper_get_extreme_radius(main_model, extreme_radius);
  // No need to get extreme radius of main_bound_model.

  for_each_steer_non_ghost_wheel(
    main_model, wheels_models,
    [&](volInt::polyhedron &wheel_model)
      {
        get_scale_helper_get_extreme_radius(&wheel_model,
                                            extreme_radius,
                                            wheel_model.offset_point());
      });

  if(weapon_slots_existence)
  {
    for(const auto &slot_data : cur_weapon_slot_data)
    {
      if(slot_data.exists)
      {
        get_extreme_radius(extreme_radius,
                           max_weapons_radius,
                           slot_data.R_slot);
      }
    }
  }
  if(debris_models)
  {
    for(auto &model : *debris_models)
    {
      get_scale_helper_get_extreme_radius(&model, extreme_radius);
    }
  }
  // No need to get extreme radius of debris_bound_models.

  rmax = extreme_radius;
  get_scale_helper_set_scale_from_rmax();
}



void wavefront_obj_to_m3d_model::get_a3d_scale_size(
  std::deque<volInt::polyhedron> *models)
{
  double extreme_radius = 0.0;

  for(auto &model : *models)
  {
    get_scale_helper_get_extreme_radius(&model, extreme_radius);
  }

  rmax = extreme_radius;
  get_scale_helper_set_scale_from_rmax();
}





void wavefront_obj_to_m3d_model::m3d_mechos_generate_bound(
  const volInt::polyhedron *main_model,
  const std::unordered_map<int, volInt::polyhedron> *wheels_models,
  const std::deque<volInt::polyhedron> *debris_models,
  volInt::polyhedron &new_main_bound,
  std::deque<volInt::polyhedron> &new_debris_bounds)
{
  volInt::polyhedron main_model_copy = *main_model;

  if(wheels_models && wheels_models->size())
  {
    std::unordered_map<int, volInt::polyhedron> wheels_to_insert =
      *wheels_models;
    merge_helper_reserve_space_in_main(&main_model_copy,
                                       &wheels_to_insert);
    merge_main_model_with_wheels(&main_model_copy,
                                 &wheels_to_insert);
  }

  volInt::model_extreme_points wheel_params_extremes;
  volInt::model_extreme_points *wheel_params_extremes_ptr = nullptr;
  if(cur_wheel_data.size())
  {
    wheel_params_extremes = get_wheel_params_extremes();
    wheel_params_extremes_ptr = &wheel_params_extremes;
  }

  new_main_bound =
    main_model_copy.generate_bound_model(
      volInt::generate_bound::model_type::mechos,
      gen_bound_layers_num,
      gen_bound_area_threshold,
      wheel_params_extremes_ptr);
  new_main_bound.wavefront_obj_path =
    file_prefix_to_path(wavefront_obj::prefix::main_bound).string();

  new_main_bound.calculate_c3d_properties();


  for(std::size_t cur_debris = 0; cur_debris < n_debris; ++cur_debris)
  {
    new_debris_bounds.push_back(
      (*debris_models)[cur_debris].generate_bound_model(
        volInt::generate_bound::model_type::other,
        gen_bound_layers_num,
        gen_bound_area_threshold));
    new_debris_bounds[cur_debris].offset_point() =
      (*debris_models)[cur_debris].offset_point();
    new_debris_bounds[cur_debris].wavefront_obj_path =
      file_prefix_to_path(wavefront_obj::prefix::debris_bound,
                          &cur_debris).string();

    new_debris_bounds[cur_debris].calculate_c3d_properties();
  }
}



void wavefront_obj_to_m3d_model::m3d_non_mechos_generate_bound(
  const volInt::polyhedron *main_model,
  volInt::polyhedron &new_main_bound)
{
  new_main_bound =
    main_model->generate_bound_model(
      volInt::generate_bound::model_type::other,
      gen_bound_layers_num,
      gen_bound_area_threshold);
  new_main_bound.wavefront_obj_path =
    file_prefix_to_path(wavefront_obj::prefix::main_bound).string();

  new_main_bound.calculate_c3d_properties();
}





void wavefront_obj_to_m3d_model::m3d_recalc_vertNorms(
  volInt::polyhedron *main_model,
  volInt::polyhedron *main_bound_model,
  std::unordered_map<int, volInt::polyhedron> *wheels_models,
  std::deque<volInt::polyhedron> *debris_models,
  std::deque<volInt::polyhedron> *debris_bound_models)
{
  main_model->recalc_vertNorms(max_smooth_angle);
  if(main_bound_model)
  {
    main_bound_model->recalc_vertNorms(max_smooth_angle);
  }

  for_each_steer_non_ghost_wheel(
    main_model, wheels_models,
    [&](volInt::polyhedron &wheel_model)
      {
        wheel_model.recalc_vertNorms(max_smooth_angle);
      });

  if(debris_models)
  {
    for(auto &&model : *debris_models)
    {
      model.recalc_vertNorms(max_smooth_angle);
    }
  }
  if(debris_bound_models)
  {
    for(auto &&model : *debris_bound_models)
    {
      model.recalc_vertNorms(max_smooth_angle);
    }
  }
}



void wavefront_obj_to_m3d_model::a3d_recalc_vertNorms(
  std::deque<volInt::polyhedron> *models)
{
  for(auto &&model : *models)
  {
    model.recalc_vertNorms(max_smooth_angle);
  }
}





std::size_t wavefront_obj_to_m3d_model::get_c3d_file_size(
  const volInt::polyhedron *model)
{
  return
    c3d::header::size +
    model->numVerts * c3d::vertex::size +
    model->numVertNorms * c3d::normal::size +
    model->numFaces *
      (c3d::polygon::general_info_size +
         (model->numVertsPerPoly * c3d::polygon::size_per_vertex) +
         c3d::polygon_sort_info::size);
}



std::size_t wavefront_obj_to_m3d_model::get_m3d_file_size(
  const volInt::polyhedron *main_model,
  const volInt::polyhedron *main_bound_model,
  const std::unordered_map<int, volInt::polyhedron> *wheels_models,
  const std::deque<volInt::polyhedron> *debris_models,
  const std::deque<volInt::polyhedron> *debris_bound_models)
{
  std::size_t size = m3d::header::size +
                     n_wheels * m3d::wheel::size +
                     m3d::weapon_slot::slots_existence_size;
  if(weapon_slots_existence)
  {
    size += m3d::weapon_slot::max_slots * m3d::weapon_slot::slot_data_size;
  }

  size += get_c3d_file_size(main_model);
  if(main_bound_model)
  {
    size += get_c3d_file_size(main_bound_model);
  }
  if(n_wheels && wheels_models)
  {
    for(std::size_t i = 0; i < n_wheels; ++i)
    {
      if(cur_wheel_data[i].steer)
      {
        size += get_c3d_file_size(&wheels_models->at(i));
      }
    }
  }
  if(n_debris && debris_models)
  {
    for(std::size_t i = 0; i < n_debris; ++i)
    {
      size += get_c3d_file_size(&(*debris_models)[i]);
    }
  }
  if(n_debris && debris_bound_models)
  {
    for(std::size_t i = 0; i < n_debris; ++i)
    {
      size += get_c3d_file_size(&(*debris_bound_models)[i]);
    }
  }
  return size;
}



std::size_t wavefront_obj_to_m3d_model::get_a3d_file_size(
  const std::deque<volInt::polyhedron> *models)
{
  std::size_t size = a3d::header::size;
  for(std::size_t cur_model = 0; cur_model < n_models; ++cur_model)
  {
    size += get_c3d_file_size(&(*models)[cur_model]);
  }
  return size;
}





void wavefront_obj_to_m3d_model::remove_polygons_helper_erase_mechos(
  volInt::polyhedron &model)
{
  model.faces.erase(
    std::remove_if(
      model.faces.begin(), model.faces.end(),
      [&](const volInt::face &poly)
      {
        if(poly.color_id == c3d::color::string_to_id::attachment_point ||
           poly.color_id == c3d::color::string_to_id::center_of_mass ||
           poly.weapon_id != volInt::invalid::weapon_id ||
           model.wheels_steer.count(poly.wheel_id) ||
           model.wheels_ghost.count(poly.wheel_id))
        {
          return true;
        }
        return false;
      }
    ),
    model.faces.end()
  );
}



void wavefront_obj_to_m3d_model::remove_polygons_helper_erase_non_mechos(
  volInt::polyhedron &model)
{
  model.faces.erase(
    std::remove_if(
      model.faces.begin(), model.faces.end(),
      [&](const volInt::face &poly)
      {
        if(poly.color_id == c3d::color::string_to_id::attachment_point ||
           poly.color_id == c3d::color::string_to_id::center_of_mass ||
           poly.weapon_id != volInt::invalid::weapon_id ||
           model.wheels_ghost.count(poly.wheel_id))
        {
          return true;
        }
        return false;
      }
    ),
    model.faces.end()
  );
}



std::vector<std::size_t>
  wavefront_obj_to_m3d_model::remove_polygons_helper_create_ind_change_map(
    std::size_t size,
    std::unordered_set<std::size_t> &verts_to_keep)
{
  std::vector<std::size_t> ret(size, 0);
  std::size_t number_skip = 0;
  for(std::size_t cur_vert = 0; cur_vert < size; ++cur_vert)
  {
    if(!verts_to_keep.count(cur_vert))
    {
      ++number_skip;
    }
    ret[cur_vert] = number_skip;
  }
  return ret;
}



void wavefront_obj_to_m3d_model::remove_polygons(
  volInt::polyhedron &model,
  remove_polygons_model model_type)
{
  std::unordered_set<std::size_t> verts_to_keep;
  std::unordered_set<std::size_t> norms_to_keep;
  verts_to_keep.reserve(model.verts.size());
  norms_to_keep.reserve(model.vertNorms.size());

  // Removing right polygons.
  if(model_type == remove_polygons_model::mechos)
  {
    remove_polygons_helper_erase_mechos(model);
  }
  else if(model_type == remove_polygons_model::non_mechos)
  {
    remove_polygons_helper_erase_non_mechos(model);
  }

  model.numFaces = model.faces.size();


  // Getting list of vertices and normals to erase.
  for(std::size_t cur_poly_n = 0; cur_poly_n < model.numFaces; ++cur_poly_n)
  {
    for(std::size_t cur_vert = 0; cur_vert < model.numVertsPerPoly; ++cur_vert)
    {
      verts_to_keep.insert(model.faces[cur_poly_n].verts[cur_vert]);
      norms_to_keep.insert(model.faces[cur_poly_n].vertNorms[cur_vert]);
    }
  }

  std::size_t cur_vert_n = 0;
  model.verts.erase(
    std::remove_if(
      model.verts.begin(), model.verts.end(),
      [&](const std::vector<double> &)
      {
        return !verts_to_keep.count(cur_vert_n++);
      }
    ),
    model.verts.end()
  );
  std::size_t cur_norm_n = 0;
  model.vertNorms.erase(
    std::remove_if(
      model.vertNorms.begin(), model.vertNorms.end(),
      [&](const std::vector<double> &)
      {
        return !norms_to_keep.count(cur_norm_n++);
      }
    ),
    model.vertNorms.end()
  );


  // Updating all existing polygons' indices
  // since vertices and normals were deleted.
  std::vector<std::size_t> new_verts_ind_change_map =
    remove_polygons_helper_create_ind_change_map(model.numVerts,
                                                 verts_to_keep);
  std::vector<std::size_t> new_norms_ind_change_map =
    remove_polygons_helper_create_ind_change_map(model.numVertNorms,
                                                 norms_to_keep);
  for(auto &&cur_poly : model.faces)
  {
    for(auto &&vert_ind : cur_poly.verts)
    {
      vert_ind -= new_verts_ind_change_map[vert_ind];
    }
    for(auto &&norm_ind : cur_poly.vertNorms)
    {
      norm_ind -= new_norms_ind_change_map[norm_ind];
    }
  }

  model.numVerts = model.verts.size();
  model.numVertNorms = model.vertNorms.size();
  model.numVertTotal = model.numFaces * model.faces[0].numVerts;
}



void create_game_lst(
  const boost::filesystem::path &input_file_path_arg,
  const boost::filesystem::path &where_to_save_arg,
  const std::string &input_file_name_error_arg,
  const std::string &output_file_name_error_arg,
  const std::unordered_map<std::string, double> *non_mechos_scale_sizes_arg)
{
  std::string orig_game_lst_data =
    read_file(input_file_path_arg,
              file_flag::binary | file_flag::read_all,
              0,
              0,
              read_all_dummy_size,
              input_file_name_error_arg);

  sicher_cfg_writer cur_cfg_writer(
    std::move(orig_game_lst_data),
    input_file_path_arg.string(),
    input_file_name_error_arg,
    non_mechos_scale_sizes_arg->size() * sicher_game_lst_int_size_increase);

  const int max_model =
    cur_cfg_writer.get_next_value<int>(van_cfg_key::game_lst::NumModel);
  const int max_size =
    cur_cfg_writer.get_next_value<int>(van_cfg_key::game_lst::MaxSize);
  for(std::size_t cur_model = 0; cur_model < max_model; ++cur_model)
  {
    const int model_num =
      cur_cfg_writer.get_next_value<int>(van_cfg_key::game_lst::ModelNum);
    if(cur_model != model_num)
    {
      throw std::runtime_error(
        input_file_name_error_arg + " file " +
        input_file_path_arg.string() + " unexpected " +
        van_cfg_key::game_lst::ModelNum + " " +
        std::to_string(model_num) + ".\n" +
        "Expected " + std::to_string(cur_model) + ".\n" +
        van_cfg_key::game_lst::ModelNum +
        " values must be placed " +
        "in order from 0 to (" + van_cfg_key::game_lst::NumModel + " - 1).\n" +
        van_cfg_key::game_lst::ModelNum + " is current model, " +
        van_cfg_key::game_lst::NumModel + " is total number of models.\n");
    }
    const std::string model_path =
      cur_cfg_writer.get_next_value<std::string>(van_cfg_key::game_lst::Name);
    const std::string model_name =
      boost::filesystem::path(model_path).stem().string();

    if((*non_mechos_scale_sizes_arg).count(model_name))
    {
      const double scale_size = (*non_mechos_scale_sizes_arg).at(model_name);
      const int scale_size_game_lst =
        round_half_to_even<double, int>(scale_size * max_size);
      cur_cfg_writer.overwrite_next_value(van_cfg_key::game_lst::Size,
                                          scale_size_game_lst,
                                          "%i");
    }
  }

  cur_cfg_writer.write_until_end();

  save_file(where_to_save_arg,
            cur_cfg_writer.out_str(),
            file_flag::binary,
            output_file_name_error_arg);
}



void create_prm(
  const boost::filesystem::path &input_file_path_arg,
  const boost::filesystem::path &where_to_save_arg,
  const std::string &input_file_name_error_arg,
  const std::string &output_file_name_error_arg,
  const double scale_size)
{
  std::string orig_prm_data =
    read_file(input_file_path_arg,
              file_flag::binary | file_flag::read_all,
              0,
              0,
              read_all_dummy_size,
              input_file_name_error_arg);

  sicher_cfg_writer cur_cfg_writer(
    std::move(orig_prm_data),
    input_file_path_arg.string(),
    input_file_name_error_arg,
    sicher_prm_float_size_increase);

  cur_cfg_writer.overwrite_next_value(
    van_cfg_key::prm::scale_size,
    scale_size,
    sicher_cfg_format::sprintf_float);
  cur_cfg_writer.write_until_end();

  save_file(where_to_save_arg,
            cur_cfg_writer.out_str(),
            file_flag::binary,
            output_file_name_error_arg);
}



void mechos_wavefront_objs_to_m3d(
  const boost::filesystem::path &input_m3d_path_arg,
  const boost::filesystem::path &output_m3d_path_arg,
  const std::string &input_file_name_error_arg,
  const std::string &output_file_name_error_arg,
  const volInt::polyhedron *example_weapon_model_arg,
  const volInt::polyhedron *weapon_attachment_point_model_arg,
  const volInt::polyhedron *center_of_mass_model_arg,
  double max_weapons_radius_arg,
  unsigned int default_c3d_material_id_arg,
  double scale_cap_arg,
  double max_smooth_angle_arg,
  std::size_t gen_bound_layers_num_arg,
  double gen_bound_area_threshold_arg,
  bitflag<obj_to_m3d_flag> flags_arg)
{
  wavefront_obj_to_m3d_model cur_vangers_model(
    input_m3d_path_arg,
    output_m3d_path_arg,
    input_file_name_error_arg,
    output_file_name_error_arg,
    example_weapon_model_arg,
    weapon_attachment_point_model_arg,
    center_of_mass_model_arg,
    max_weapons_radius_arg,
    default_c3d_material_id_arg,
    scale_cap_arg,
    max_smooth_angle_arg,
    gen_bound_layers_num_arg,
    gen_bound_area_threshold_arg,
    flags_arg,
    nullptr);
  cur_vangers_model.mechos_wavefront_objs_to_m3d();
}



volInt::polyhedron weapon_wavefront_objs_to_m3d(
  const boost::filesystem::path &input_m3d_path_arg,
  const boost::filesystem::path &output_m3d_path_arg,
  const std::string &input_file_name_error_arg,
  const std::string &output_file_name_error_arg,
  const volInt::polyhedron *weapon_attachment_point_arg,
  const volInt::polyhedron *center_of_mass_model_arg,
  unsigned int default_c3d_material_id_arg,
  double scale_cap_arg,
  double max_smooth_angle_arg,
  std::size_t gen_bound_layers_num_arg,
  double gen_bound_area_threshold_arg,
  bitflag<obj_to_m3d_flag> flags_arg,
  std::unordered_map<std::string, double> *non_mechos_scale_sizes_arg)
{
  wavefront_obj_to_m3d_model cur_vangers_model(
    input_m3d_path_arg,
    output_m3d_path_arg,
    input_file_name_error_arg,
    output_file_name_error_arg,
    nullptr,
    weapon_attachment_point_arg,
    center_of_mass_model_arg,
    0.0,
    default_c3d_material_id_arg,
    scale_cap_arg,
    max_smooth_angle_arg,
    gen_bound_layers_num_arg,
    gen_bound_area_threshold_arg,
    flags_arg,
    non_mechos_scale_sizes_arg);
  return cur_vangers_model.weapon_wavefront_objs_to_m3d();
}



void animated_wavefront_objs_to_a3d(
  const boost::filesystem::path &input_m3d_path_arg,
  const boost::filesystem::path &output_m3d_path_arg,
  const std::string &input_file_name_error_arg,
  const std::string &output_file_name_error_arg,
  const volInt::polyhedron *center_of_mass_model_arg,
  unsigned int default_c3d_material_id_arg,
  double scale_cap_arg,
  double max_smooth_angle_arg,
  bitflag<obj_to_m3d_flag> flags_arg,
  std::unordered_map<std::string, double> *non_mechos_scale_sizes_arg)
{
  wavefront_obj_to_m3d_model cur_vangers_model(
    input_m3d_path_arg,
    output_m3d_path_arg,
    input_file_name_error_arg,
    output_file_name_error_arg,
    nullptr,
    nullptr,
    center_of_mass_model_arg,
    0.0,
    default_c3d_material_id_arg,
    scale_cap_arg,
    max_smooth_angle_arg,
    option::default_val::gen_bound_layers_num,
    option::default_val::gen_bound_area_threshold,
    flags_arg,
    non_mechos_scale_sizes_arg);
  cur_vangers_model.animated_wavefront_objs_to_a3d();
}



void other_wavefront_objs_to_m3d(
  const boost::filesystem::path &input_m3d_path_arg,
  const boost::filesystem::path &output_m3d_path_arg,
  const std::string &input_file_name_error_arg,
  const std::string &output_file_name_error_arg,
  const volInt::polyhedron *center_of_mass_model_arg,
  unsigned int default_c3d_material_id_arg,
  double scale_cap_arg,
  double max_smooth_angle_arg,
  std::size_t gen_bound_layers_num_arg,
  double gen_bound_area_threshold_arg,
  bitflag<obj_to_m3d_flag> flags_arg,
  std::unordered_map<std::string, double> *non_mechos_scale_sizes_arg)
{
  wavefront_obj_to_m3d_model cur_vangers_model(
    input_m3d_path_arg,
    output_m3d_path_arg,
    input_file_name_error_arg,
    output_file_name_error_arg,
    nullptr,
    nullptr,
    center_of_mass_model_arg,
    0.0,
    default_c3d_material_id_arg,
    scale_cap_arg,
    max_smooth_angle_arg,
    gen_bound_layers_num_arg,
    gen_bound_area_threshold_arg,
    flags_arg,
    non_mechos_scale_sizes_arg);
  cur_vangers_model.other_wavefront_objs_to_m3d();
}



} // namespace helpers
} // namespace tractor_converter
