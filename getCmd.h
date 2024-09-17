#include <string>
// using namespace std;
std::string filePath(std::string inc, char jd, int n)
{

    // for(int i = 0; i < inc.length(); i++)
    // {
    //     cout<<i<<":"<<inc[i]<<endl;
    // }
    bool isQuotationMarks = false;
    int number = 0, i_xb = 0;
    char lastCh = 0;
    for (int i = 0; i < inc.length(); i++)
    {
        for (int j = 1; j <= 2; j++)
        {
            char now_if_ch = (j == 1 ? '\"' : jd);
            if (inc[i] == now_if_ch && !isQuotationMarks && i < inc.length() - 1 && ((inc[i + 1] != now_if_ch && inc[i - 1] != now_if_ch) || (inc[i + 1] != now_if_ch && inc[i - 1] == now_if_ch)))
            {
                lastCh = inc[i];
                isQuotationMarks = true;
                number++;
                i_xb = i + 1;
            }
            else if (inc[i] == now_if_ch && lastCh == now_if_ch && isQuotationMarks && number == n)
            {
                return inc.substr(i_xb, i - i_xb);
            }
            else if (inc[i] == now_if_ch && lastCh == now_if_ch && isQuotationMarks && number != n)
            {
                if (j == 1)
                {
                    isQuotationMarks = false;
                }
                else
                {
                    if (inc[i + 1] != jd && inc[i - 1] != jd)
                    {
                        isQuotationMarks = true;
                        number++;
                    }
                    else
                    {
                        isQuotationMarks = false;
                    }
                    i_xb = i + 1;
                }
            }
        }
    }
    if (number + 1 == n)
    {
        return inc.substr(i_xb);
    }
    return "";
}