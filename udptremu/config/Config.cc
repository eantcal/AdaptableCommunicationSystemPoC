//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#include "Config.h"

/* -------------------------------------------------------------------------- */

void Config::describe(std::ostream &os)
{
    for (const auto &session : _data)
    {

        os << "[" << session.first << "]" << std::endl;

        for (const auto &kv : session.second)
        {
            os << kv.first << "=";

            if (kv.second.second == Type::STRING)
                os << "\"";

            os << kv.second.first;

            if (kv.second.second == Type::STRING)
                os << "\"";
                
            os << std::endl;
        }
    }
}

/* -------------------------------------------------------------------------- */

void Config::add(const std::string &name, const std::string &value, Type type)
{
    _data[_namespace][name] = {value, type};
    ++_lines;

    _rowData.push_back({_namespace, name, value, type});
}

/* -------------------------------------------------------------------------- */

void Config::compile()
{

    for (auto &line : _rowData)
    {
        TextTokenizer tt(/*kv.second.first*/ line.value, 0,
                         " ",              // blanks
                         "\n",             // lf
                         "$.",             // single-char ops
                         {},               // multichar-ops
                         '(', ')',         // sub expr
                         "\"", "\"", '\\', // string
                         {""});

        TokenList tl;
        tt.getTokenList(tl);

        // var=$namespace.var
        if (tl.size() == 4 || tl.size() == 2)
        {
            auto prefix_op = (tl)[0];

            if (prefix_op.type() != TokenClass::OPERATOR || prefix_op.identifier() != "$")
            {
                _data[line.namespaceId][line.identifier] = {line.value, line.valueType};
                continue;
            }

            auto namespace_id = tl.size() == 2 ? line.namespaceId : tl[1].identifier();
            auto namespace_va = (tl)[tl.size() == 2 ? 1 : 3];

            if (tl.size() > 2 && ((tl)[2].type() != TokenClass::OPERATOR || (tl)[2].identifier() != "."))
            {
                _data[line.namespaceId][line.identifier] = {line.value, line.valueType};
                continue;
            }

            // If the namespace is _env the variable is assumed as an envirnonment variable ($varname)
            // For example:
            //    role = $_env.ROLE  # assign to global.role the content of $ROLE
            if (namespace_id == "_env")
            {
                auto envval = std::getenv(namespace_va.original_value().c_str());
                if (envval)
                {
                    _data[line.namespaceId][line.identifier] = {envval, Config::Type::STRING};
                }
            }
            else
            {
                auto it = _data.find(namespace_id);
                if (it != _data.end())
                {
                    auto value = it->second[namespace_va.identifier()];
                    line.value = value.first;
                    line.valueType = value.second;
                    _data[line.namespaceId][line.identifier] = value;
                }
            }
        }
        else
        {
            _data[line.namespaceId][line.identifier] = {line.value, line.valueType};
        }
    }
}

/* -------------------------------------------------------------------------- */

std::list<std::string> Config::getAttrList(const std::string &name) const
{
    auto it = data().find(_namespace);

    std::list<std::string> list;
    if (it != data().end())
    {
        const auto &namespace_data = it->second;
        auto nsit = namespace_data.find(name);
        if (nsit != namespace_data.end())
        {
            TextTokenizer tk(nsit->second.first, 0, ", \t\r\n");
            TokenList tl;
            tk.getTokenList(tl);

            while (!tl.empty())
            {
                if (tl.begin()->type() == TokenClass::IDENTIFIER)
                {
                    list.push_back(tl.begin()->identifier());
                }
                --tl;
            }
        }
    }

    return list;
}

/* -------------------------------------------------------------------------- */

std::string Config::getAttr(const std::string &name) const
{
    const auto &data = getAttrList(name);
    return data.empty() ? std::string() : *(data.begin());
}
