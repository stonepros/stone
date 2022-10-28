#include "acconfig.h"
#include "stone_ver.h"
#include "CompressionPluginBrotli.h"
#include "common/stone_context.h"


const char *__stone_plugin_version()
{
  return STONE_GIT_NICE_VER;
}

int __stone_plugin_init(StoneContext *cct,
                       const std::string& type,
                       const std::string& name)
{
  PluginRegistry *instance = cct->get_plugin_registry();
  return instance->add(type, name, new CompressionPluginBrotli(cct));
}

