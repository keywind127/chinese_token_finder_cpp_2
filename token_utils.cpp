#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <string>
#include <cmath>
using namespace std;
class BasicTokenData
{
public:
    size_t occurrences = 1;  unordered_map <wchar_t, size_t> prevChars;  unordered_map <wchar_t, size_t> postChars;
};
class BasicTokenScore
{
public:
    size_t occurrences;  float pmiScore, enlScore, enrScore;
    BasicTokenScore() {  ;  }
    BasicTokenScore(size_t occurrences, float pmiScore, float enlScore, float enrScore)
        : occurrences(occurrences), pmiScore(pmiScore), enlScore(enlScore), enrScore(enrScore) {  ;  }
};
class ChineseStringTokenizer
{
public:
    wstring sourceString;  size_t strLen;  static size_t threshOCC;  wchar_t nullChar = L'X';
    static float defaultPMI, defaultENL, defaultENR, threshPMI, threshENL, threshENR;

    ChineseStringTokenizer(wchar_t sourceString[], size_t strLen)
        : sourceString(wstring(sourceString, &(sourceString[strLen]))), strLen(strLen) {  ;  }
    inline bool valid_index(size_t index) {  return ((index < strLen) && (sourceString[index] != nullChar));  }
    inline bool valid_token(wstring && token) {  return (token.find(nullChar) >= token.length());  }
    inline static void configure_default_scores(float defaultPMI, float defaultENL, float defaultENR)
    {
        ChineseStringTokenizer::defaultPMI = defaultPMI;  ChineseStringTokenizer::defaultENL = defaultENL;
        ChineseStringTokenizer::defaultENR = defaultENR;
    }
    inline static void configure_thresh_values(size_t threshOCC, float threshPMI, float threshENL, float threshENR)
    {
        ChineseStringTokenizer::threshOCC = threshOCC;  ChineseStringTokenizer::threshPMI = threshPMI;
        ChineseStringTokenizer::threshENL = threshENL;  ChineseStringTokenizer::threshENR = threshENR;
    }
    void tokenize_sliding_window(unordered_map <wstring, BasicTokenData> & slidingWindowResult, size_t windowSize = 1)
    {
        size_t tempIdx, startIdx, maxIndx = strLen - windowSize;  wstring tempToken;  wchar_t tempChar;
        for (startIdx = 0; startIdx <= maxIndx; ++startIdx)
        {
            tempToken = sourceString.substr(startIdx, windowSize);
            if (slidingWindowResult.find(tempToken) != slidingWindowResult.end())
            {
                slidingWindowResult[tempToken].occurrences += 1;
            }
            else
            {
                slidingWindowResult[tempToken] = BasicTokenData();
            }
            tempIdx = startIdx - 1;
            if (valid_index(tempIdx))
            {
                tempChar = sourceString[tempIdx];
                if (slidingWindowResult[tempToken].prevChars.find(tempChar) != slidingWindowResult[tempToken].prevChars.end())
                {
                    slidingWindowResult[tempToken].prevChars[tempChar] += 1;
                }
                else
                {
                    slidingWindowResult[tempToken].prevChars[tempChar] = 1;
                }
            }
            tempIdx = startIdx + windowSize;
            if (valid_index(tempIdx))
            {
                tempChar = sourceString[tempIdx];
                if (slidingWindowResult[tempToken].postChars.find(tempChar) != slidingWindowResult[tempToken].postChars.end())
                {
                    slidingWindowResult[tempToken].postChars[tempChar] += 1;
                }
                else
                {
                    slidingWindowResult[tempToken].postChars[tempChar] = 1;
                }
            }
        }
    }
    void compute_token_scores(unordered_map <wstring, BasicTokenScore> & tokenScoreResult, unordered_map <wstring, BasicTokenData> && slidingWindowResult)
    {
        for (unordered_map<wstring,BasicTokenData>::iterator it = slidingWindowResult.begin(); it != slidingWindowResult.end(); ++it)
        {
            tokenScoreResult[it->first] = BasicTokenScore(it->second.occurrences, defaultPMI, defaultENL, defaultENR);
        }
    }
    float compute_token_pmi(unordered_map <wstring, BasicTokenScore> & tokenScoreResultPrev, const wstring & tempToken, size_t occurrences)
    {
        constexpr float maxPMI = 999999;  float minPmi = maxPMI, tempPmi;
        wstring splitL, splitR;  size_t splitIdx, strLen = tempToken.length();
        for (splitIdx = 1; splitIdx < strLen; ++splitIdx)
        {
            splitL = tempToken.substr(0, splitIdx);
            splitR = tempToken.substr(splitIdx, strLen - splitIdx);
            if ((tokenScoreResultPrev.find(splitL) != tokenScoreResultPrev.end()) &&
                (tokenScoreResultPrev.find(splitR) != tokenScoreResultPrev.end())) {
                minPmi = min(minPmi, log10(occurrences * this->strLen /
                    (float) tokenScoreResultPrev[splitL].occurrences /
                        (float) tokenScoreResultPrev[splitR].occurrences));
            }
        }
        return ((minPmi == maxPMI) ? (0) : (minPmi));
    }
    float compute_entropy(unordered_map <wchar_t, size_t> & sideChars)
    {
        float temp, total = 0, score = 0;
        for (unordered_map<wchar_t,size_t>::iterator it = sideChars.begin(); it != sideChars.end(); ++it)
        {
            total += it->second;
        }
        for (unordered_map<wchar_t,size_t>::iterator it = sideChars.begin(); it != sideChars.end(); ++it)
        {
            temp = it->second / total;
            score -= (temp * log10(temp));
        }
        return score;
    }
    void compute_token_scores(unordered_map <wstring, BasicTokenScore> & tokenScoreResult,
                              unordered_map <wstring, BasicTokenScore> & tokenScoreResultPrev,
                              unordered_map <wstring, BasicTokenData> && slidingWindowResult) {
        for (unordered_map<wstring,BasicTokenData>::iterator it = slidingWindowResult.begin(); it != slidingWindowResult.end(); ++it)
        {
            tokenScoreResult[it->first] = BasicTokenScore(
                it->second.occurrences, compute_token_pmi(tokenScoreResultPrev, it->first, it->second.occurrences),
                    compute_entropy(it->second.prevChars), compute_entropy(it->second.postChars));
        }
    }
    unordered_map <wstring, BasicTokenScore> filter_by_pmi(unordered_map <wstring, BasicTokenScore> & tokenScoreResult)
    {
        unordered_map <wstring, BasicTokenScore> filteredTokenScoreResult;
        for (unordered_map<wstring,BasicTokenScore>::iterator it = tokenScoreResult.begin(); it != tokenScoreResult.end(); ++it)
        {
            if (it->second.pmiScore >= threshPMI)
            {
                filteredTokenScoreResult[it->first] = it->second;
            }
        }
        return filteredTokenScoreResult;
    }
    unordered_map <wstring, BasicTokenScore> filter_tokens(unordered_map <wstring, BasicTokenScore> & tokenScoreResult)
    {
        unordered_map <wstring, BasicTokenScore> filteredTokenScoreResult;
        for (unordered_map<wstring,BasicTokenScore>::iterator it = tokenScoreResult.begin(); it != tokenScoreResult.end(); ++it)
        {
            if ((it->second.pmiScore    >= threshPMI) &&
                (it->second.enlScore    >= threshENL) &&
                (it->second.enrScore    >= threshENR) &&
                (it->second.occurrences >= threshOCC)) {
                filteredTokenScoreResult[it->first] = it->second;
            }
        }
        return filteredTokenScoreResult;
    }
};
float ChineseStringTokenizer::defaultPMI = 0,
      ChineseStringTokenizer::defaultENL = 0,
      ChineseStringTokenizer::defaultENR = 0,
      ChineseStringTokenizer::threshPMI  = 0,
      ChineseStringTokenizer::threshENL  = 0,
      ChineseStringTokenizer::threshENR  = 0;
size_t ChineseStringTokenizer::threshOCC = 0;
class ChineseTokenManager
{
public:
    unordered_map <wstring, BasicTokenScore> tokenScores;
    inline static void configure_default_scores(float defaultPMI, float defaultENL, float defaultENR)
    {
        ChineseStringTokenizer::configure_default_scores(defaultPMI, defaultENL, defaultENR);
    }
    inline static void configure_thresh_values(size_t threshOCC, float threshPMI, float threshENL, float threshENR)
    {
        ChineseStringTokenizer::configure_thresh_values(threshOCC, threshPMI, threshENL, threshENR);
    }
    void find_tokens(wchar_t sourceString[], size_t strLen, size_t windowSize = 1)
    {
        ChineseStringTokenizer tokenizer(sourceString, strLen);
        unordered_map <wstring, BasicTokenData>
    }
};
int main()
{


    return 0;
}
