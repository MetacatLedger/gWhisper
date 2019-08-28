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
#include <vector>
#include <memory>

#include <libArgParse/ParsedElement.hpp>

namespace ArgParse
{
    class ParsedElement;

    typedef struct Coordinate
    {
        uint32_t depth;// depth
        uint32_t step;//step to right from root.
        uint32_t index; // step to right from root node of a sub tree

    } Coordinate;

    class DocumentInfo
    {
        public:
            DocumentInfo():
                m_parseElement(nullptr),
                m_max_step(0)
            {
            }

            DocumentInfo(ParsedElement * f_parseElement):
                m_parseElement(f_parseElement),
                m_max_step(0)
            {
            }

            std::vector<Coordinate> getPath() const
            {
                return m_path;
            }

            void updatePath(std::vector<Coordinate> f_path)
            {
                m_path = f_path;
            }

            uint32_t getMaxStep() const
            {
                return m_max_step;
            }

            void setMaxStep(uint32_t f_max_step)
            {
                m_max_step = f_max_step;
            }

            ///how many steps to right from the root
            void calculateStepFromRoot()
            {
                for(int i = 0; i != m_path.size(); ++i)
                {
                    if(i>0)
                    {
                        for(int j = 0; j<=i; ++j)
                        {
                                m_path[i].step += m_path[j].index;
                        }
                    }
                    if (m_path[i].step > m_max_step)
                    {
                        m_max_step = m_path[i].step;
                    }
                }
            }

            ParsedElement * getParsedElement() const
            {
                return m_parseElement;
            }

            void printPath() const
            {
                std::cout << "(depth, index): ";
                for(auto & node: m_path)
                {
                    std::cout << "(" << node.depth << ", " << node.index << ")->";
                }
                std::cout << m_parseElement->getGrammarElement()->getDocument() << std::endl;

                std::cout << "(depth, step): ";
                for(auto & node: m_path)
                {
                    std::cout << "(" << node.depth << ", " << node.step << ")->";
                }
                std::cout << m_parseElement->getGrammarElement()->getDocument() << std::endl;
            }

        private:
            //not used
            template<typename... Args>
            void searchChilden(ParsedElement * f_parseElement, std::string & f_out_document, Args...f_compared_string)
            {
                std::string childDoc = f_parseElement->getGrammarElement()->getDocument();
                if(!childDoc.empty())
                {
                    std::string delims = "\r\n\t";
                    childDoc.erase(0, childDoc.find_first_not_of(delims));
                    childDoc.erase(childDoc.find_last_not_of(delims) + 1);

                    std::vector<std::string> list = {f_compared_string...};
                    for(auto& compared : list)
                    {
                        if(compared.find(childDoc) != std::string::npos)
                        {
                            return;
                        }
                        else
                        {
                            f_out_document = childDoc;
                        }
                    }
                }
                else
                {
                    auto& childen = f_parseElement->getChildren();
                    if(childen.size() > 0)
                    {
                        for(auto& child: childen)
                        {
                            searchChilden(child.get(), f_out_document, std::forward<Args>(f_compared_string)...);
                        }
                    }
                }
            }

        private:
            std::vector<Coordinate> m_path;
            uint32_t m_max_step;
            ParsedElement * m_parseElement;
    };

    //not used
    void transToMatrix(const std::vector<DocumentInfo> & f_documents);

    void abstractDocTree(ParsedElement * f_parseElement, std::vector<DocumentInfo> & f_out_documents, std::vector<Coordinate> f_path, uint32_t f_depth, uint32_t f_index);

    std::string searchDocument(ParsedElement * f_parseElement, bool f_debug);
}