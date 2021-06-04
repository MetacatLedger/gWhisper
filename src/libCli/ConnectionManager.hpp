// Copyright 2019 IBM Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <fstream>
//#include <readline/readline.h>

#include <grpcpp/grpcpp.h>
#include <grpcpp/security/credentials.h>
#include <libArgParse/ArgParse.hpp>

#include <third_party/gRPC_utils/proto_reflection_descriptor_database.h>

namespace cli
{
    /// List of gRpc connection infomation
    typedef struct ConnList
    {
        std::shared_ptr<grpc::Channel> channel = nullptr;
        std::shared_ptr<grpc::ProtoReflectionDescriptorDatabase> descDb = nullptr;
        std::shared_ptr<grpc::protobuf::DescriptorPool> descPool = nullptr;

    } ConnList;

    /// Class to manage and resuse connection information, singleton pattern.
    class ConnectionManager
    {
    public:
        ConnectionManager(const ConnectionManager &) = delete;
        ConnectionManager &operator=(const ConnectionManager &) = delete;

    private:
        ConnectionManager() {}
        ~ConnectionManager() {}

        ArgParse::ParsedElement parseTree;
        enum class Connection
        {
            insecure,
            secure,
            dflt
        };

        Connection connectionType;

        int connectionStatus;

    public:
        /// Only use a single connection instance
        static ConnectionManager &getInstance()
        {
            static ConnectionManager connectionManager;
            return connectionManager;
        }

        std::string connectionEnumToString(Connection f_connectionType)
        {
            switch (f_connectionType)
            {
            case Connection::secure:
                return ("secure");
                break;
            case Connection::insecure:
                return ("insecure");
                break;
            default:
                return ("default");
            }
        }

        /// Get CommandlineArgs from Main. Needed for Deciding which Channel to build

        void getConnectionStatus(const int f_connectionStatus)
        // Is static here ok? For me Connection status is const, that is why i used static here. If static is not ok, gereate connectionmanager-object in gwhisper.cpp
        {
            if (f_connectionStatus)
                // if ((f_connectionStatus == connection::insecure) || (f_connectionStatus == connection::secure) || (f_connectionStatus == connection::default))
                if ((f_connectionStatus == 0) || (f_connectionStatus == 1) || (f_connectionStatus == 2))
                {
                    connectionStatus = f_connectionStatus;
                    std::cout << "Connection Status: " << f_connectionStatus << std::endl;
                }
                else
                {
                    std::cerr << "Could not set Connection Status" << std::endl;
                }
        }

        int getConnectionStatus()
        {
            //not sure if getter is needed.
            return connectionStatus;
        }

        /// To get the channel according to the server address. If the cached map doesn't contain the channel, create the connection list and update the map.
        /// @param f_serverAddress Service Addresses with Port, described in gRPC string format "hostname:port".
        /// @returns the channel of the corresponding server address.
        std::shared_ptr<grpc::Channel> getChannel(std::string f_serverAddress, ArgParse::ParsedElement &f_parseTree)
        //should f_parseTree be a pointer?
        {
            if (!findChannelByAddress(f_serverAddress))
            {
                //Connection connectionType; Singleton richtig angesprochen?
                ConnectionManager::parseTree = f_parseTree;
                /* 
                if (f_parseTree.findFirstChild("--noSsl") != "")
                {
                    std::cout << "In If of first Child" << std::endl;
                    ConnectionManager::connectionType = Connection::insecure;
                    std::cout << "Connection Status: " << ConnectionManager::connectionEnumToString(Connection::insecure) << std::endl;
                }
                else if (f_parseTree.findFirstChild("--ssl") != "")
                {
                    ConnectionManager::connectionType = Connection::secure;
                }
                else
                {
                    ConnectionManager::connectionType = Connection::dflt;
                } */

                registerConnection(f_serverAddress);
            }
            return connections[f_serverAddress].channel;
        }

        /// To get the gRpc DescriptorDatabase according to the server address. If the cached map doesn't contain the channel, create the connection list and update the map.
        /// @param f_serverAddress Service Addresses with Port, described in gRPC string format "hostname:port".
        /// @returns the gRpc DescriptorDatabase of the corresponding server address.
        std::shared_ptr<grpc::ProtoReflectionDescriptorDatabase> getDescDb(std::string f_serverAddress)
        {
            if (!findDescDbByAddress(f_serverAddress))
            {
                if (connections[f_serverAddress].channel)
                {
                    connections[f_serverAddress].descDb = std::make_shared<grpc::ProtoReflectionDescriptorDatabase>(connections[f_serverAddress].channel);
                    connections[f_serverAddress].descPool = std::make_shared<grpc::protobuf::DescriptorPool>(connections[f_serverAddress].descDb.get());
                }
                else
                {
                    registerConnection(f_serverAddress);
                }
            }
            return connections[f_serverAddress].descDb;
        }
        /// To get the gRpc DescriptorPool according to the server address. If the cached map doesn't contain the channel, create the connection list and update the map.
        /// @param f_serverAddress Service Addresses with Port, described in gRPC string format "hostname:port".
        /// @returns the gRpc DescriptorPool of the corresponding server address.
        std::shared_ptr<grpc::protobuf::DescriptorPool> getDescPool(std::string f_serverAddress)
        {

            if (!findDescPoolByAddress(f_serverAddress))
            {
                if (connections[f_serverAddress].channel)
                {
                    connections[f_serverAddress].descDb = std::make_shared<grpc::ProtoReflectionDescriptorDatabase>(connections[f_serverAddress].channel);
                    connections[f_serverAddress].descPool = std::make_shared<grpc::protobuf::DescriptorPool>(connections[f_serverAddress].descDb.get());
                }
                else
                {
                    registerConnection(f_serverAddress);
                }
            }
            return connections[f_serverAddress].descPool;
        }

    private:
        // Cached map of the gRpc connection information for resuing the channel, descriptor Database and DatabasePool
        std::unordered_map<std::string, ConnList> connections;
        // Check if the cached map contains the channel of the given server address or not.
        bool findChannelByAddress(std::string f_serverAddress)
        {
            if (connections.find(f_serverAddress) != connections.end())
            {
                if (connections[f_serverAddress].channel != nullptr)
                {
                    return true;
                }
            }
            return false;
        }
        // Check if the cached map contains the gRpc DescriptorDatabase of given the server address or not.
        bool findDescDbByAddress(std::string f_serverAddress)
        {
            if (connections.find(f_serverAddress) != connections.end())
            {
                if (connections[f_serverAddress].descDb != nullptr)
                {
                    return true;
                }
            }
            return false;
        }
        // Check if the cached map contains the gRpc DescriptorPool of the given server address or not.
        bool findDescPoolByAddress(std::string f_serverAddress)
        {
            if (connections.find(f_serverAddress) != connections.end())
            {
                if (connections[f_serverAddress].descPool != nullptr)
                {
                    return true;
                }
            }
            return false;
        }
        /// To register the gRpc connection information of a given server address.
        /// Connection List contains: Channel, DescriptorDatabase and DescriptorPool as value.
        /// @param f_serverAddress Service Addresses with Port, described in gRPC string format "hostname:port" as key of the cached map.
        void registerConnection(std::string f_serverAddress)
        {

            ConnList connection;
            std::shared_ptr<grpc::ChannelCredentials> creds;

            //std::shared_ptr<grpc::ChannelCredentials> channelCreds = getCredentials(clientCertPath, clientKeyPath, serverCertPath);
            std::shared_ptr<grpc::ChannelCredentials> channelCreds;

            if (ConnectionManager::parseTree.findFirstChild("ssl") != "")
            {
                // check if user provides Cerificates / Keys
                bool clientCertOption = (ConnectionManager::parseTree.findFirstChild("OptionClientCert") != "");
                bool clientKeyOption = (ConnectionManager::parseTree.findFirstChild("OptionClientKey") != "");
                bool serverCertOption = (ConnectionManager::parseTree.findFirstChild("OptionServerCert") != "");

                std::string sslClientCertPath = ConnectionManager::parseTree.findFirstChild("FileClientCert");
                std::string sslClientKeyPath = ConnectionManager::parseTree.findFirstChild("FileClientKey");
                std::string sslServerCertPath = ConnectionManager::parseTree.findFirstChild("FileServerCert");

                if (clientCertOption && clientKeyOption && serverCertOption)
                {
                    std::cout << "ClientCert: Entered SSL=TRUE" << std::endl;

                    std::cout << "CREATE SECURE CAHNNEL WITH USER-PROVIDED CREDENTIALS" << std::endl;
                    channelCreds = generateSSLCredentials(sslClientCertPath, sslClientKeyPath, sslServerCertPath);
                    connection.channel = grpc::CreateChannel(f_serverAddress, channelCreds);
                }
                else
                {
                    // ask if user wants to provide Files
                    std::string userInput;
                    std::cout << "Do you want to provide Certs/ Keys for SSL-Authentication [Y]/[N]?" << std::endl;
                    std::cin >> userInput;

                    if (userInput == "Y" || userInput == "y")
                    {

                        if (!clientCertOption)
                        {
                            sslClientCertPath = getFile("Client-Cert");
                        }

                        if (!clientKeyOption)
                        {
                            sslClientKeyPath = getFile("Client-Key");
                        }

                        if (!serverCertOption)
                        {
                            sslServerCertPath = getFile("Server-Cert");
                        }

                        std::cout << "CREATE SECURE CAHNNEL USER-PROVIDED CREDENTIALS" << std::endl;
                        channelCreds = generateSSLCredentials(sslClientCertPath, sslClientKeyPath, sslServerCertPath);
                        connection.channel = grpc::CreateChannel(f_serverAddress, channelCreds);
                    }
                    else if (userInput == "N" || userInput == "n")
                    {
                        //const std::string dfltClientCertPath = ConnectionManager::parseTree.findFirstChild("FileClientCert");
                        //const std::string dfltClientKeyPath = ConnectionManager::parseTree.findFirstChild("FileClientKey");
                        //const std::string dfltServerCertPath = ConnectionManager::parseTree.findFirstChild("ServerClientCert");

                        // Connect with default Credentials
                        std::cout << "CREATE SECURE CAHNNEL WITH DEFAULT CREDENTIALS" << std::endl;
                        //channelCreds = generateSSLCredentials(dfltClientCertPath, dfltClientKeyPath, dfltServerCertPath);
                        channelCreds = generateSSLCredentials(sslClientCertPath, sslClientKeyPath, sslServerCertPath);
                        checkCredentials(channelCreds);
                        connection.channel = grpc::CreateChannel(f_serverAddress, channelCreds);
                    }
                    else
                    {
                        std::cout << "Invalid User Input" << std::endl;
                    }
                }
            }
            else
            { //create insecure channel
                std::cout << "CREATE INSECURE CAHNNEL" << std::endl;
                connection.channel = grpc::CreateChannel(f_serverAddress, grpc::InsecureChannelCredentials());
            }

            std::cout << "*****************************************************************************************************" << std::endl;
            std::cout << "Connected to Server: " << f_serverAddress << std::endl;
            std::cout << "Created Channel: " << connection.channel << std::endl;

            connection.descDb = std::make_shared<grpc::ProtoReflectionDescriptorDatabase>(connection.channel);
            connection.descPool = std::make_shared<grpc::protobuf::DescriptorPool>(connection.descDb.get());
            connections[f_serverAddress] = connection;
        }

        /// get key/certs for SSL/TLS-connection from user Input
        std::string getFile(std::string fileID)
        {

            std::string userInput;
            std::cout << "Please Provide location of " << fileID << " : " << std::endl;
            //userInput = readline(" ");
            std::cin >> userInput;
            return userInput;
        }

        void checkCredentials(std::shared_ptr<grpc::ChannelCredentials> f_channelCreds)
        {
            if (!f_channelCreds)
            {
                std::cout << "No / Wrong Channel Credentials" << std::endl;
            }
            else
            {
                std::cout << "*****************************************************************************************************" << std::endl;
                std::cout << "Channel Credentials: " << f_channelCreds << std::endl;
            }
        }

        /// Get Key-Cert Pairs from Files and use them as credentials for SSL/TLS Channel
        std::shared_ptr<grpc::ChannelCredentials> generateSSLCredentials(const std::string f_sslClientCertPath, const std::string f_sslClientKeyPath, const std::string f_sslServerCertPath)
        {
            std::string clientKey = readFromFile(f_sslClientKeyPath);
            std::string clientCert = readFromFile(f_sslClientCertPath);
            std::string serverCert = readFromFile(f_sslServerCertPath);
            std::cout << "client Cert File: " << clientCert << std::endl;
            std::cout << "client Key File: " << clientKey << std::endl;
            std::cout << "Server Cert File: " << serverCert << std::endl;

            grpc::SslCredentialsOptions sslOpts;
            sslOpts.pem_private_key = clientKey;
            sslOpts.pem_cert_chain = clientCert;
            sslOpts.pem_root_certs = serverCert;

            std::shared_ptr<grpc::ChannelCredentials> creds = grpc::SslCredentials(sslOpts);
            std::cout << "SSL-Creds " << creds << std::endl;

            return creds;
        }

        /// Function for reading and returning credentials (Key, Cert) from file
        std::string readFromFile(const std::string f_path)
        {
            std::ifstream credFile(f_path);
            const char *file = f_path.c_str();
            if (file)
            {

                std::string str{std::istreambuf_iterator<char>(credFile),
                                std::istreambuf_iterator<char>()};

                // std::cout << "Channel: File content of " << f_path << ": " << str << std::endl;
                return str;
            }
            else
            {
                std::cout << "No cert/key found at " << f_path << std::endl;
                std::cout << "Failed to build secure channel";
                exit(EXIT_FAILURE);
            }
        }
    };
}
