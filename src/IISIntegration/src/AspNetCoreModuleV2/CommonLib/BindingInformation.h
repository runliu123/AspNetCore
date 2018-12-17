// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the MIT License. See License.txt in the project root for license information.

#pragma once

#include <string>
#include "ConfigurationSource.h"
#include "StringHelpers.h"
#include "WebConfigConfigurationSource.h"

#define CS_SITE_SECTION                         L"system.applicationHost/sites"
#define CS_SITE_NAME                            L"name"
#define CS_SITE_BINDINGS                        L"bindings"
#define CS_SITE_BINDING_INFORMATION             L"bindingInformation"
#define CS_SITE_BINDING_INFORMATION_ALL_HOSTS   L"*"
#define CS_SITE_BINDING_PROTOCOL                L"protocol"
#define CS_SITE_BINDING_PROTOCOL_HTTPS          L"https"
#define CS_SITE_BINDING_INFORMATION_DELIMITER   L':'

class BindingInformation
{
public:
    BindingInformation(std::wstring protocol, std::wstring host, std::wstring port)
    {
        m_protocol = protocol;
        m_host = host;
        m_port = port;
    }

    std::wstring& QueryProtocol()
    {
        return m_protocol;
    }

    std::wstring& QueryPort()
    {
        return m_port;
    }

    std::wstring& QueryHost()
    {
        return m_host;
    }

    static
    std::vector<BindingInformation>
    Load(const ConfigurationSource &configurationSource, const IHttpSite& pSite)
    {
        std::vector<BindingInformation> items;

        const std::wstring runningSiteName = pSite.GetSiteName();

        auto const siteSection = configurationSource.GetRequiredSection(CS_SITE_SECTION);
        auto sites = siteSection->GetCollection();
        for (const auto& site: sites)
        {
            auto siteName = site->GetRequiredString(CS_SITE_NAME);
            if (equals_ignore_case(runningSiteName, siteName))
            {
                auto bindings = site->GetRequiredSection(CS_SITE_BINDINGS)->GetCollection();
                for (const auto& binding : bindings)
                {
                    // Expected format:
                    // IP:PORT:HOST
                    // where IP or HOST can be empty
                    const auto information = binding->GetRequiredString(CS_SITE_BINDING_INFORMATION);
                    const auto firstColon = information.find(CS_SITE_BINDING_INFORMATION_DELIMITER) + 1;
                    const auto lastColon = information.find_last_of(CS_SITE_BINDING_INFORMATION_DELIMITER);

                    std::wstring host;
                    // Check that : is not the last character
                    if (lastColon != information.length() + 1)
                    {
                        auto const afterLastColon = lastColon + 1;
                        host = information.substr(afterLastColon, information.length() - afterLastColon);
                    }
                    if (host.length() == 0)
                    {
                        host = CS_SITE_BINDING_INFORMATION_ALL_HOSTS;
                    }

                    items.emplace_back(
                        binding->GetRequiredString(CS_SITE_BINDING_PROTOCOL),
                        host,
                        information.substr(firstColon, lastColon - firstColon)
                        );
                }
            }
        }

        return items;
    }

    static
    std::wstring Format(const std::vector<BindingInformation> & bindings, const std::wstring & basePath)
    {
        std::wstring result;

        for (auto binding : bindings)
        {
            result += binding.QueryProtocol() + L"://" + binding.QueryHost() + L":" + binding.QueryPort() + basePath + L";";
        }

        return result;
    }

    static
    std::wstring GetHttpsPort(const std::vector<BindingInformation> & bindings)
    {
        std::wstring selectedPort;
        for (auto binding : bindings)
        {
            if (equals_ignore_case(binding.QueryProtocol(), CS_SITE_BINDING_PROTOCOL_HTTPS))
            {
                const auto bindingPort = binding.QueryPort();
                if (selectedPort.empty())
                {
                    selectedPort = binding.QueryPort();
                }
                else if (selectedPort != bindingPort)
                {
                    // If there are multiple endpoints configured return empty port
                    return L"";
                }
            }
        }
        return selectedPort;
    }

private:
    std::wstring                   m_protocol;
    std::wstring                   m_port;
    std::wstring                   m_host;
};
