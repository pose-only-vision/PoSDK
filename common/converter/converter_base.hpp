/**
 * @file converter_base.hpp
 * @brief Base converter class and basic conversion functions | 转换器基类和基础转换函数
 * @details Provides basic data type conversion functionality and converter base class definition
 *          提供基础数据类型转换功能和转换器基类定义
 *
 * @copyright Copyright (c) 2024 Qi Cai
 * Licensed under the Mozilla Public License Version 2.0
 */

#ifndef _CONVERTER_BASE_
#define _CONVERTER_BASE_

#include <po_core/types.hpp>
#include <po_core/po_logger.hpp>
#include <string>
#include <iostream>

namespace PoSDK
{
    namespace Converter
    {

        using namespace PoSDK::types;

        //------------------------------------------------------------------------------
        // Converter base class | 转换器基类
        //------------------------------------------------------------------------------

        /**
         * @brief Base converter class | 转换器基类
         * @details Base class for all concrete converters, providing basic interfaces
         *          所有具体转换器的基类，提供基本接口
         */
        class ConverterBase
        {
        public:
            virtual ~ConverterBase() = default;

            // Common converter interfaces can be added later as needed
            // 后续看情况，可以添加一些通用的转换接口
        };

    } // namespace Converter
} // namespace PoSDK

#endif // _CONVERTER_BASE_
