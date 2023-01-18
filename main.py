from token_utils import ChineseTokenFinder
import datetime, pprint, os 
if (__name__ == "__main__"):

    thresh_occ = 3
    thresh_pmi = 1.5
    thresh_enl = 0.2
    thresh_enr = 0.2

    string = open(os.path.join(os.path.dirname(__file__), "string.txt"), "r", encoding = "utf-8").read()
    string = ChineseTokenFinder.mask_string(string)
    string = ChineseTokenFinder.simplify_mask(string)
    SOT    = datetime.datetime.now()
    with ChineseTokenFinder() as tokenizer:
        tokenizer.set_thresh_enl(thresh_enl).set_thresh_enr(
            thresh_enr).set_thresh_occ(thresh_occ).set_thresh_pmi(thresh_pmi)
        for length in range(1, 11):
            tokenizer.analyze_new_string(string)
            tokenizer.increment_window_size()
        result = tokenizer.extract_token_scores()
        result = ChineseTokenFinder.default_sort(result)
        with open(os.path.join(os.path.dirname(__file__), "tokens.txt"), "w", encoding = "utf-8") as WF:
            for (token, scoreOCC, scorePMI, scoreENL, scoreENR) in result:
                WF.write(f"{token}\n")
    EOT    = datetime.datetime.now()
    #pprint.pprint(result)
    print("Time: {} seconds".format((EOT - SOT).total_seconds()))