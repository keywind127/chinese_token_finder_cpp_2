# Chinese New Token Finder Documentation

### Example

    from token_utils import ChineseTokenFinder
    # threshold variables to filter out mis-matches
    thresh_occ = 1
    thresh_pmi = 0.0
    thresh_enl = 0.0
    thresh_enr = 0.0
    #
    string1 = "每天早上醒來我都想吃派LMAO！"
    string2 = "人家常說天下沒有白吃的午餐"
    string1 = ChineseTokenFinder.mask_string(string1) # mark non-Chinese characters as faulty
    string1 = ChineseTokenFinder.simplify_mask(string1) # reduce non-Chinese substring to maxlen 2
    string2 = ChineseTokenFinder.mask_string(string2)
    string2 = ChineseTokenFinder.simplify_mask(string2)
    with ChineseTokenFinder() as tokenizer:
        tokenizer.set_thresh_enl(thresh_enl)
        tokenizer.set_thresh_enr(thresh_enr)
        tokenizer.set_thresh_occ(thresh_occ)
        tokenizer.set_thresh_pmi(thresh_pmi)
        for length in range(1, 6):
            tokenizer.analyze_new_string(string1)
            tokenizer.analyze_new_string(string2)
            tokenizer.increment_window_size()
        result = tokenizer.extract_token_scores()
        result = ChineseTokenFinder.default_sort(result)
        __import__("pprint").pprint(result)