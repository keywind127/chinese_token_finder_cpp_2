from typing import Union, Tuple, Dict, Type, List 
import pprint, ctypes, os
class ChineseTokenFinder:
    LIBRARY_NAME = os.path.join(os.path.dirname(__file__), "token_utils2.so")

    library                                 = ctypes.cdll.LoadLibrary(LIBRARY_NAME)
    library.initialize.restype              = ctypes.c_void_p 
    library.destroy.argtypes                = [ ctypes.c_void_p ]
    library.increment_window_size.argtypes  = [ ctypes.c_void_p ]
    library.analyze_chinese_string.argtypes = [ 
        ctypes.c_void_p, ctypes.c_wchar_p, ctypes.c_size_t ]
    library.compute_token_scores.argtypes   = [ ctypes.c_void_p ]
    library.configure_null_char.argtypes    = [ ctypes.c_wchar ]
    library.find_quantity.argtypes          = [ ctypes.c_void_p, ctypes.c_void_p ]
    library.find_quantity.restype           = ctypes.c_size_t
    library.extract_token_scores.argtypes   = [ 
        ctypes.c_void_p, ctypes.c_wchar_p, ctypes.c_void_p, 
            ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p ]
    library.configure_threshold.argtypes    = [ ctypes.c_void_p, 
        ctypes.c_size_t, ctypes.c_float, ctypes.c_float, ctypes.c_float ]

    null_char = "X"

    @staticmethod 
    def default_sort(token_list : List[ Tuple[ str, int, float, float, float ] ]) -> List[ Tuple[ str, int, float, float, float ] ]:
        return list(reversed(sorted(token_list, key = lambda x : ((len(x[0]) != 1), x[1], x[2], x[3], x[4]))))

    @classmethod 
    def mask_string(class_, source_string : str) -> str:
        return "".join(map(lambda x : ((x) if (ord("\u4E00") <= ord(x) <= ord("\u9FFF")) 
            else (class_.null_char)), source_string))

    @classmethod 
    def simplify_mask(class_, source_string : str) -> str:
        def valid_char(args_pair : Tuple[ int, str ]) -> bool:
            (index, char) = args_pair
            nonlocal source_string
            def valid_index(index : int) -> bool:
                nonlocal source_string 
                return ((0 <= index) and (index < len(source_string)))
            if (char != class_.null_char):
                return True 
            (idx_l, idx_r) = (index - 1, index + 1)
            if ((valid_index(idx_l) and (source_string[idx_l] == class_.null_char)) and 
                (valid_index(idx_r) and (source_string[idx_r] == class_.null_char))):
                return False 
            return True 
        binary_mask = list(map(valid_char, enumerate(source_string)))
        return "".join(i[1] for i in filter(lambda i : binary_mask[i[0]], enumerate(source_string)))

    @classmethod 
    def configure_null_char(class_, null_char : str) -> Type[ "ChineseTokenFinder" ]:
        class_.null_char = null_char 
        class_.library.configure_null_char(null_char.encode("utf-8"))
        return class_ 

    def __init__(self) -> None:
        self.tokenizer   = self.library.initialize()
        self.window_size = 1
        self.thresh_occ  = 0
        self.thresh_pmi  = 0
        self.thresh_enl  = 0
        self.thresh_enr  = 0

    def __delete__(self) -> None:
        self.library.destroy(self.tokenizer)

    def __enter__(self) -> "ChineseTokenFinder":
        return self 
    
    def __exit__(self, *args, **kwargs) -> None:
        pass 

    def __update_thresh(self) -> "ChineseTokenFinder":
        self.library.configure_threshold(self.tokenizer, 
            self.thresh_occ, self.thresh_pmi, self.thresh_enl, self.thresh_enr)
        return self 

    def set_thresh_occ(self, thresh_occ : int) -> "ChineseTokenFinder":
        self.thresh_occ = thresh_occ 
        return self.__update_thresh()

    def set_thresh_pmi(self, thresh_pmi : float) -> "ChineseTokenFinder":
        self.thresh_pmi = thresh_pmi 
        return self.__update_thresh()

    def set_thresh_enl(self, thresh_enl : float) -> "ChineseTokenFinder":
        self.thresh_enl = thresh_enl 
        return self.__update_thresh()
    
    def set_thresh_enr(self, thresh_enr : float) -> "ChineseTokenFinder":
        self.thresh_enr = thresh_enr 
        return self.__update_thresh()

    def increment_window_size(self) -> int:
        self.window_size += 1
        self.library.increment_window_size(self.tokenizer)
        return self.window_size 

    def analyze_new_string(self, source_string : str) -> bool:
        str_len = len(source_string)
        if (str_len < self.window_size):
            return False 
        self.library.analyze_chinese_string(self.tokenizer, 
            (ctypes.c_wchar * str_len)(*source_string), str_len)
        return True 

    def extract_token_scores(self) -> List[ Tuple[ str, int, float, float, float ] ]:
        def tokenize_string(tokens_string : str, tokens_indices : List[ int ]) -> List[ str ]:
            tokens_indices = [ 0 ] + tokens_indices# + [ len(tokens_string) ]
            return [
                tokens_string[tokens_indices[idx-1]:tokens_indices[idx]]
                    for idx in range(1, len(tokens_indices))
            ]
        self.library.compute_token_scores(self.tokenizer)
        len_token_string = (ctypes.c_size_t * 1)()
        num_tokens       = self.library.find_quantity(self.tokenizer, len_token_string)
        len_token_string = int(len_token_string[0])
        tokens_string    = (ctypes.c_wchar  * len_token_string)()
        tokens_quantity  = (ctypes.c_size_t *       num_tokens)()
        tokens_indices   = (ctypes.c_size_t *       num_tokens)()
        tokens_pmi_score = (ctypes.c_float  *       num_tokens)()
        tokens_enl_score = (ctypes.c_float  *       num_tokens)()
        tokens_enr_score = (ctypes.c_float  *       num_tokens)()
        self.library.extract_token_scores(self.tokenizer, tokens_string, tokens_indices, 
            tokens_quantity, tokens_pmi_score, tokens_enl_score, tokens_enr_score)
        tokens_string    = "".join((char for char in tokens_string))
        tokens_quantity  = list(tokens_quantity)
        tokens_indices   = list(tokens_indices)
        tokens_pmi_score = list(tokens_pmi_score)
        tokens_enl_score = list(tokens_enl_score)
        tokens_enr_score = list(tokens_enr_score)
        tokens_string    = tokenize_string(tokens_string, tokens_indices)
        return list(zip(tokens_string, tokens_quantity, 
            tokens_pmi_score, tokens_enl_score, tokens_enr_score))

if (__name__ == "__main__"):

    string1 = "中國是世界上最強的國家！！！！！！我覺得這是一場誤會"

    string1 = ChineseTokenFinder.mask_string(string1)

    string1 = ChineseTokenFinder.simplify_mask(string1)

    with ChineseTokenFinder() as tokenizer:

        for length in range(1, len(string1) + 2):

            print("Analyzing str{}: {}".format(length, tokenizer.analyze_new_string(string1)))

            tokenizer.increment_window_size()

        result = tokenizer.extract_token_scores()

        #pprint.pprint(result)