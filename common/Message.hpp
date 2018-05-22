//
//  Chat
//

#pragma once

#include <string>
#include <cereal/types/string.hpp>

struct Message
{
    enum class Type
    {
        LOGIN,
        TEXT
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
