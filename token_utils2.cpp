#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <locale>
#include <string>
#include <cmath>
using namespace std;
class TokenAnalysis
{
public:
    size_t occurrences = 1;  unordered_map <wchar_t, size_t> prevChars;  unordered_map <wchar_t, size_t> postChars;
};
class TokenScores
{
public:
    size_t occurrences;  float scorePMI, scoreENL, scoreENR;
    TokenScores() {  ;  }
    TokenScores(size_t occurrences, float scorePMI, float scoreENL, float scoreENR)
        : occurrences(occurrences), scorePMI(scorePMI), scoreENL(scoreENL), scoreENR(scoreENR) {  ;  }
};
class ChineseTokenizer
{
public:
    wchar_t * sourceString;  size_t strLen;  static wchar_t nullChar;
    ChineseTokenizer(wchar_t sourceString[], size_t strLen) : sourceString(sourceString), strLen(strLen) {  ;  }
    inline bool valid_index(size_t index) {  return ((index < strLen) && (sourceString[index] != nullChar));  }
    inline bool valid_token(const wstring & token) {  return (token.find(nullChar) >= token.length());  }
    inline void analyze_side_chars(unordered_map <wchar_t, size_t> & tempMap, wchar_t & tempChr)
    {
        tempMap[tempChr] = ((tempMap.find(tempChr) != tempMap.end()) ? (tempMap[tempChr] + 1) : (1));
    }
    unordered_map <wstring, TokenAnalysis> analyze_with_sliding_window(size_t windowSize)
    {
        unordered_map <wstring, TokenAnalysis> analysisResult;
        size_t startIdx, endIdx = strLen - windowSize, tempIdx;  wstring tempToken;
        for (startIdx = 0; startIdx <= endIdx; ++startIdx)
        {
            tempToken = wstring(&(sourceString[startIdx]), &(sourceString[startIdx + windowSize]));
            if (valid_token(tempToken))
            {
                if (analysisResult.find(tempToken) != analysisResult.end())
                {
                    analysisResult[tempToken].occurrences += 1;
                }
                else
                {
                    analysisResult[tempToken] = TokenAnalysis();
                }
                tempIdx = startIdx - 1;
                if (valid_index(tempIdx))
                {
                    analyze_side_chars(analysisResult[tempToken].prevChars, sourceString[tempIdx]);
                }
                tempIdx = startIdx + windowSize;
                if (valid_index(tempIdx))
                {
                    analyze_side_chars(analysisResult[tempToken].postChars, sourceString[tempIdx]);
                }
            }
        }
        return analysisResult;
    }
};
wchar_t ChineseTokenizer::nullChar = L'X';
class TokenManager
{
public:
    unordered_map <wstring, TokenAnalysis> basicAnalysis;  unordered_map <wstring, TokenScores> finalAnalysis;
    size_t windowSize = 1, stringLen = 0, tokenStringLen = 0, threshOCC = 0;
    float defaultPMI = 99999, defaultENL = 99999, defaultENR = 99999, threshPMI = 0, threshENL = 0, threshENR = 0;
    inline void increment_window_size() {  windowSize += 1;  }
    void configure_threshold(size_t threshOCC, float threshPMI, float threshENL, float threshENR)
    {
        this->threshOCC = threshOCC;  this->threshPMI = threshPMI;  this->threshENL = threshENL;  this->threshENR = threshENR;
    }
    void analyze_chinese_string(wchar_t sourceString[], size_t strLen)
    {
        stringLen += strLen;
        unordered_map <wstring, TokenAnalysis> tempAnalysis = \
            ChineseTokenizer(sourceString, strLen).analyze_with_sliding_window(windowSize);
        wstring tempToken;
        for (unordered_map<wstring,TokenAnalysis>::iterator it = tempAnalysis.begin(); it != tempAnalysis.end(); ++it)
        {
            tempToken = it->first;
            if (basicAnalysis.find(tempToken) != basicAnalysis.end())
            {
                basicAnalysis[tempToken].occurrences += it->second.occurrences;
                unordered_map <wchar_t, size_t>::iterator it2, endIdx = it->second.prevChars.end();
                for (it2 = it->second.prevChars.begin(); it2 != endIdx; ++it2)
                {
                    if (basicAnalysis[tempToken].prevChars.find(it2->first) != basicAnalysis[tempToken].prevChars.end())
                    {
                        basicAnalysis[tempToken].prevChars[it2->first] += it2->second;
                    }
                    else
                    {
                        basicAnalysis[tempToken].prevChars[it2->first] = it2->second;
                    }
                }
                endIdx = it->second.postChars.end();
                for (it2 = it->second.postChars.begin(); it2 != endIdx; ++it2)
                {
                    if (basicAnalysis[tempToken].postChars.find(it2->first) != basicAnalysis[tempToken].postChars.end())
                    {
                        basicAnalysis[tempToken].postChars[it2->first] += it2->second;
                    }
                    else
                    {
                        basicAnalysis[tempToken].postChars[it2->first] = it2->second;
                    }
                }
            }
            else
            {
                basicAnalysis[tempToken] = it->second;
            }
        }
    }
    float compute_pmi(wstring token)
    {
        constexpr float MAX_PMI = 99999;  float minPMI = MAX_PMI;
        size_t splitIdx, strLen = token.length();  wstring tokenL, tokenR;
        for (splitIdx = 1; splitIdx < strLen; ++splitIdx)
        {
            tokenL = token.substr(0, splitIdx);
            tokenR = token.substr(splitIdx, strLen - splitIdx);
            if ((basicAnalysis.find(tokenL) != basicAnalysis.end()) && (basicAnalysis.find(tokenR) != basicAnalysis.end()))
            {
                minPMI = min(minPMI, log10(basicAnalysis[token].occurrences * stringLen
                    / (float) basicAnalysis[tokenL].occurrences / (float) basicAnalysis[tokenR].occurrences));
            }
        }
        return ((minPMI == MAX_PMI) ? (0) : (minPMI));
    }
    float compute_entropy(unordered_map <wchar_t, size_t> & tokenData)
    {
        float total = 0, score = 0, tempv;
        for (unordered_map<wchar_t,size_t>::iterator it = tokenData.begin(); it != tokenData.end(); ++it)
        {
            total += it->second;
        }
        for (unordered_map<wchar_t,size_t>::iterator it = tokenData.begin(); it != tokenData.end(); ++it)
        {
            tempv = (it->second) / total;
            score -= (tempv * log10(tempv));
        }
        return score;
    }
    void compute_token_scores()
    {
        unordered_map<wstring,TokenAnalysis>::iterator it, endIdx = basicAnalysis.end();  TokenScores tokenScore;
        for (it = basicAnalysis.begin(); it != endIdx; ++it)
        {
            tokenStringLen += it->first.length();
            if (it->first.length() == 1)
            {
                finalAnalysis[it->first] = TokenScores(it->second.occurrences, defaultPMI, defaultENL, defaultENR);
            }
            else
            {
                tokenScore = TokenScores(it->second.occurrences, compute_pmi(it->first),
                    compute_entropy(it->second.prevChars), compute_entropy(it->second.postChars));
                if ((tokenScore.occurrences >= threshOCC) &&
                    (tokenScore.scorePMI    >= threshPMI) &&
                    (tokenScore.scoreENL    >= threshENL) &&
                    (tokenScore.scoreENR    >= threshENR)) {
                    finalAnalysis[it->first] = tokenScore;
                }
            }
        }
    }
    void extract_token_scores(wchar_t  tokenString[],
                               size_t tokenIndices[],
                               size_t    scoresOCC[],
                                float    scoresPMI[],
                                float    scoresENL[],
                                float    scoresENR[]) {
        size_t tokenIdx = 0, stringIdx = 0;  wstring tempToken;
        for (unordered_map<wstring,TokenScores>::iterator it = finalAnalysis.begin(); it != finalAnalysis.end(); ++it, ++tokenIdx)
        {
            scoresPMI[tokenIdx] = it->second.scorePMI;
            scoresENL[tokenIdx] = it->second.scoreENL;
            scoresENR[tokenIdx] = it->second.scoreENR;
            scoresOCC[tokenIdx] = it->second.occurrences;
            tempToken = it->first;
            memcpy(&(tokenString[stringIdx]), &(tempToken[0]), sizeof(wchar_t) * tempToken.length());
            stringIdx += tempToken.length();
            tokenIndices[tokenIdx] = stringIdx;
        }
    }
};
void print_token_analysis(unordered_map <wstring, TokenAnalysis> & tokenAnalysis)
{
    wprintf(L"Token Analysis\n");
    for (unordered_map<wstring,TokenAnalysis>::iterator it1 = tokenAnalysis.begin(); it1 != tokenAnalysis.end(); ++it1)
    {
        wprintf(L"\t%ls:\n", it1->first.c_str());
        wprintf(L"\t\tOCCU: %d\n", (int) it1->second.occurrences);
        wprintf(L"\t\tPREV:\n");
        for (unordered_map<wchar_t,size_t>::iterator it2 = it1->second.prevChars.begin(); it2 != it1->second.prevChars.end(); ++it2)
        {
            wprintf(L"\t\t\t%lc: %d\n", it2->first, (int) it2->second);
        }
        wprintf(L"\t\tPOST:\n");
        for (unordered_map<wchar_t,size_t>::iterator it2 = it1->second.postChars.begin(); it2 != it1->second.postChars.end(); ++it2)
        {
            wprintf(L"\t\t\t%lc: %d\n", it2->first, (int) it2->second);
        }
    }
}
void print_token_scores(unordered_map <wstring, TokenScores> & tokenScores)
{
    wprintf(L"Token Scores\n");
    for (unordered_map<wstring,TokenScores>::iterator it = tokenScores.begin(); it != tokenScores.end(); ++it)
    {
        wprintf(L"\t%ls:\n", it->first.c_str());
        wprintf(L"\t\tOCC: %d\n", it->second.occurrences);
        wprintf(L"\t\tPMI: %f\n", it->second.scorePMI);
        wprintf(L"\t\tENL: %f\n", it->second.scoreENL);
        wprintf(L"\t\tENR: %f\n", it->second.scoreENR);
    }
}
extern "C"
{
    TokenManager * initialize()
    {
        return new TokenManager();
    }
    void destroy(TokenManager * tokenManager)
    {
        delete tokenManager;
    }
    void increment_window_size(TokenManager * tokenManager)
    {
        tokenManager->increment_window_size();
    }
    void analyze_chinese_string(TokenManager * tokenManager, wchar_t sourceString[], size_t strLen)
    {
        tokenManager->analyze_chinese_string(sourceString, strLen);
    }
    void compute_token_scores(TokenManager * tokenManager)
    {
        tokenManager->compute_token_scores();
    }
    void configure_null_char(wchar_t nullChar)
    {
        ChineseTokenizer::nullChar = nullChar;
    }
    size_t find_quantity(TokenManager * tokenManager, size_t * strLen)
    {
        *strLen = tokenManager->tokenStringLen;
        return tokenManager->finalAnalysis.size();
    }
    void extract_token_scores(TokenManager * tokenManager, wchar_t  tokenString[],
                                                            size_t tokenIndices[],
                                                            size_t    scoresOCC[],
                                                             float    scoresPMI[],
                                                             float    scoresENL[],
                                                             float    scoresENR[]) {
        tokenManager->extract_token_scores(tokenString,
            tokenIndices, scoresOCC, scoresPMI, scoresENL, scoresENR);
    }
    void configure_threshold(TokenManager * tokenManager, size_t threshOCC, float threshPMI, float threshENL, float threshENR)
    {
        tokenManager->configure_threshold(threshOCC, threshPMI, threshENL, threshENR);
    }
}
int main()
{
    system("chcp 936");
    setlocale(LC_ALL, "chs");
    TokenManager tokenManager;
    wchar_t strOne[] = L"中國是世界第美國中二大經濟體中";  size_t lenOne = wcslen(strOne);
    wchar_t strTwo[] = L"美國不像大家口中國中那麼弱中間";  size_t lenTwo = wcslen(strTwo);
    for (int i = 0; i < 4; ++i)
    {
        tokenManager.analyze_chinese_string(strOne, lenOne);
        tokenManager.analyze_chinese_string(strTwo, lenTwo);

        tokenManager.increment_window_size();
    }
    //print_token_analysis(tokenManager.basicAnalysis);
    tokenManager.compute_token_scores();
    print_token_scores(tokenManager.finalAnalysis);
    return 0;
}
