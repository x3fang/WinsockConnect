#include <stdio.h>
#include <iostream>
#include <sstream>
#include <map>
#include <queue>
#include <conio.h>
using std::cin;
using std::cout;
using std::endl;
using std::fstream;
using std::ifstream;
using std::ios;
using std::istringstream;
using std::map;
using std::ofstream;
using std::queue;
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
            for (int i = 0; i < wanIpRule.size(); i++)
            {
                if (wanIp == wanIpRule[i].ruleData && wanIpRule[i].ruleOperatorType == 2)
                {
                    wanIpMatch = true;
                    break;
                }
                else if (wanIp != wanIpRule[i].ruleData && wanIpRule[i].ruleOperatorType == 1)
                {
                    wanIpMatch = true;
                    break;
                }
            }
        else if (wanIpRule.size() > 0 && wanIp.empty())
            return false;
        else
            wanIpMatch = true;
        // cout << "Number 1 pass\n";
        // lanIp matching
        if (lanIpRule.size() > 0 && !lanIp.empty())
            for (int i = 0; i < lanIpRule.size(); i++)
            {
                if (lanIp == lanIpRule[i].ruleData && lanIpRule[i].ruleOperatorType == 2)
                {
                    lanIpMatch = true;
                    break;
                }
                else if (lanIp != lanIpRule[i].ruleData && lanIpRule[i].ruleOperatorType == 1)
                {
                    lanIpMatch = true;
                    break;
                }
            }
        else if (lanIpRule.size() > 0 && lanIp.empty())
            return false;
        else
            lanIpMatch = true;
        // cout << "Number 2 pass\n";
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
        // cout << "Number 3 pass\n";
        // cout << wanIpMatch << " " << lanIpMatch << " " << portMatch << endl;
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
int main()
{
    filter f;
    f.addRule(filter::wanip, "==", "192.168.1.1");
    f.addRule(filter::lanip, "!=", "192.168.1.2");
    f.addRule(filter::port, ">", "80");
    cout << f.matching("192.168.1.1", "192.168.1.2", "80") << endl;
    cout << f.matching("192.168.1.1", "192.168.1.2", "82") << endl;
    cout << f.matching("192.168.1.1", "192.168.1.2", "79") << endl;

    cout << f.matching("192.168.1.1", "192.168.1.5", "80") << endl;
    cout << f.matching("192.168.1.1", "192.168.1.5", "82") << endl;
    cout << f.matching("192.168.1.1", "192.168.1.5", "79") << endl;

    cout << f.matching("192.168.1.4", "192.168.1.2", "80") << endl;
    cout << f.matching("192.168.1.4", "192.168.1.2", "82") << endl;
    cout << f.matching("192.168.1.4", "192.168.1.2", "79") << endl;
    getch();
}