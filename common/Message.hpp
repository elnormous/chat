//
//  Chat
//

#pragma once

#include <string>
#include "cereal/types/string.hpp"

namespace chat
{
    struct Message
    {
        enum class Type: uint8_t
        {
            LOGIN,
            TEXT,
            STATUS
        };

        Type type;
        std::string nickname;
        std::string body;

        template <class Archive>
        void serialize(Archive& archive)
        {
            archive(type, nickname, body);
        }
    };
}
