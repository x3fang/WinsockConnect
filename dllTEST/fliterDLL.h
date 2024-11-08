#ifndef FILTER_H
#define FILTER_H
#include <iostream>
#include <vector>
#include <cstdlib>
#include <string>
#include <map>
using std::map;
using std::string;
using std::to_string;
using std::vector;

class filter
{
private:
    bool notMatching;
    struct rule
    {

        string ruleData;
        int ruleOperatorType; //!= == > < >= <=
                              // 1  2 3 4 5  6
        rule(string _ruleData, int _ruleOperatorType)
        {
            ruleData = _ruleData;
            ruleOperatorType = _ruleOperatorType;
        }
    };
    vector<rule> wanIpRule, lanIpRule, portRule;
    map<string, int> StringToIntForruleOperator =
        {
            {"!=", 1},
            {"==", 2},
            {">", 3},
            {"<", 4},
            {">=", 5},
            {"<=", 6}};
    bool GetnotMatchingState()
    {
        return this->notMatching;
    }

public:
    enum ruleDataType
    {
        wanip = 1,
        lanip = 2,
        port = 3,
        all = 4
    };
    const bool matching(string wanIp, string lanIp, string port)
    {
        if (notMatching)
        {
            return true;
        }
        bool wanIpMatch = false;
        bool lanIpMatch = false;
        bool portMatch = false;

        if ((wanIpRule.size() == 0 && lanIpRule.size() == 0 && portRule.size() == 0)

            || (wanIp.empty() && lanIp.empty() && port.empty()))
        {
            return false;
        }
        // wanIp matching
        if (wanIpRule.size() > 0 && !wanIp.empty())
            for (int i = 1; i <= wanIpRule.size(); i++)
            {
                if (wanIp.find(wanIpRule[i].ruleData) != string::npos)
                {
                    if (wanIpRule[i].ruleOperatorType == 2)
                    {
                        wanIpMatch = true;
                        break;
                    }
                    else
                    {
                        return false;
                    }
                }
            }
        else if (wanIpRule.size() > 0 && wanIp.empty())
            return false;
        else
            wanIpMatch = true;

        // lanIp matching
        if (lanIpRule.size() > 0 && !lanIp.empty())
            for (int i = 1; i <= lanIpRule.size(); i++)
            {
                if (lanIp.find(lanIpRule[i - 1].ruleData) != string::npos)
                {
                    if (lanIpRule[i].ruleOperatorType == 2)
                    {
                        lanIpMatch = true;
                        break;
                    }
                    else
                    {
                        return false;
                    }
                }
            }
        else if (lanIpRule.size() > 0 && lanIp.empty())
            return false;
        else
            lanIpMatch = true;

        // port matching
        if (portRule.size() > 0 && !portRule.empty())
            for (int i = 1; i <= portRule.size(); i++)
            {
                int iPort = stoi(port);
                int iPortRule = stoi(portRule[i - 1].ruleData);
                switch (portRule[i - 1].ruleOperatorType)
                {
                case 1: //!=
                    if (port == portRule[i - 1].ruleData)
                        return false;
                    portMatch = true;
                    break;
                case 2: //==
                    if (port == portRule[i - 1].ruleData)
                        portMatch = true;
                    else
                        return false;
                    break;
                case 3: //>
                    if (iPort > iPortRule)
                        portMatch = true;
                    else
                        return false;
                    break;
                case 4: //<
                    if (iPort < iPortRule)
                        portMatch = true;
                    else
                        return false;
                case 5: //>=
                    if (iPort >= iPortRule)
                        portMatch = true;
                    else
                        return false;
                case 6: //<=
                    if (iPort <= iPortRule)
                        portMatch = true;
                    else
                        return false;
                }
            }
        else if (portRule.size() > 0 && portRule.empty())
            return false;
        else
            portMatch = true;
        return wanIpMatch && lanIpMatch && portMatch;
    }
    bool addRule(ruleDataType ruleDataType, string ruleOperatorType, string ruleData)
    {
        vector<rule> *ruleVector;
        switch (ruleDataType)
        {
        case wanip:
            if (ruleOperatorType == "!=" || ruleOperatorType == "==")
            {
                ruleVector = &wanIpRule;
                break;
            }
            else
                return false;
        case lanip:
            if (ruleOperatorType == "!=" || ruleOperatorType == "==")
            {
                ruleVector = &lanIpRule;
                break;
            }
            else
                return false;
        case port:
            if (ruleOperatorType == "!=" || ruleOperatorType == "==" || ruleOperatorType == ">" || ruleOperatorType == "<" || ruleOperatorType == ">=" || ruleOperatorType == "<=")
            {
                ruleVector = &portRule;
                break;
            }
            else
                return false;
        case all:
            notMatching = true;
            return true;
        default:
            return false;
        }
        ruleVector->push_back(rule{ruleData, StringToIntForruleOperator[ruleOperatorType]});
        ruleVector = NULL;
        return true;
    }
};
#endif