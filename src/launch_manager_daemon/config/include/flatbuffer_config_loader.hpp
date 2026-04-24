// *******************************************************************************
// Copyright (c) 2026 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
//
// SPDX-License-Identifier: Apache-2.0
// *******************************************************************************

#ifndef FLATBUFFER_CONFIG_LOADER_HPP
#define FLATBUFFER_CONFIG_LOADER_HPP

#include "config_loader.hpp"

#include <cstdint>
#include <vector>

namespace score::launch_manager::config {

class FlatbufferConfigLoader : public IConfigLoader {
  public:
    score::cpp::expected<Config, Error> load(const score::filesystem::Path& path) override;

  private:
    score::cpp::expected<std::vector<uint8_t>, Error> readFile(const score::filesystem::Path& path);
    score::cpp::expected<Config, Error> parseBuffer(const std::vector<uint8_t>& buffer);
};

}  // namespace score::launch_manager::config

#endif  // FLATBUFFER_CONFIG_LOADER_HPP
