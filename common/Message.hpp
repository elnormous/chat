//
//  Chat
//

#pragma once

#include <string>

struct Message
{
    enum class Type
    {
        LOGIN,
        CLIENT_MESSAGE,
        SERVER_MESSAGE
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
