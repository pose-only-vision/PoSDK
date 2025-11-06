// This file is part of PoSDK, an Pose-only Multiple View Geometry C++ library.

// Copyright (c) 2021 Qi Cai.

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "DataExample.hpp"
#include <fstream>
#include <iostream>

namespace PoSDKPlugin
{
    // Implementation can go here if needed
    // 如果需要，实现可以放在这里

} // namespace PoSDKPlugin

// Register plugin with factory
// 向工厂注册插件
REGISTRATION_PLUGIN(PoSDKPlugin::DataExample, "data_example")
