#pragma once

#include "MetaTaskLoader.h"

namespace mc_tasks
{

template<typename T,
  typename std::enable_if<(!std::is_same<T, MetaTask>::value) && std::is_base_of<MetaTask, T>::value, int>::type>
std::shared_ptr<T> MetaTaskLoader::load(const mc_rbdyn::Robots & robots,
                                        const std::string & file)
{
  return cast<T>(load(robots, file));
}

template<typename T,
  typename std::enable_if<(!std::is_same<T, MetaTask>::value) && std::is_base_of<MetaTask, T>::value, int>::type>
std::shared_ptr<T> MetaTaskLoader::load(const mc_rbdyn::Robots & robots,
                                        const mc_rtc::Configuration & config)
{
  return cast<T>(load(robots, config));
}

template<typename T>
std::shared_ptr<T> MetaTaskLoader::cast(const MetaTaskPtr & mt)
{
  auto ret = std::dynamic_pointer_cast<T>(mt);
  if(!ret)
  {
    LOG_ERROR_AND_THROW(std::runtime_error, "The task stored in the JSON object is not of the requested type")
  }
  return ret;
}

}
