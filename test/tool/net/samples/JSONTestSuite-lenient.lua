
-- 
-- Nicolas Seriot's JSONTestSuite
-- https://github.com/nst/JSONTestSuite
-- commit d64aefb55228d9584d3e5b2433f720ea8fd00c82
-- 
-- MIT License
-- 
-- Copyright (c) 2016 Nicolas Seriot
-- 
-- Permission is hereby granted, free of charge, to any person obtaining a copy
-- of this software and associated documentation files (the "Software"), to deal
-- in the Software without restriction, including without limitation the rights
-- to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
-- copies of the Software, and to permit persons to whom the Software is
-- furnished to do so, subject to the following conditions:
-- 
-- The above copyright notice and this permission notice shall be included in all
-- copies or substantial portions of the Software.
-- 
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
-- IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
-- FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
-- AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
-- LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
-- OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
-- SOFTWARE.
-- 

-- these test cases are prefixed with n_, but
-- ljson doesn't reject any of them as invalid
-- we run these tests anyway just to ensure that
-- no segfaults occurs while parsing these cases

-- from fail4.lua

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_single_space.json
assert(nil ~= pcall(DecodeJson, [[  ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_object_with_trailing_garbage.json
assert(nil ~= pcall(DecodeJson, [[ {"a":"b"}# ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_object_two_commas_in_a_row.json
assert(nil ~= pcall(DecodeJson, [[ {"a":"b",,"c":"d"} ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_object_trailing_comment_slash_open_incomplete.json
assert(nil ~= pcall(DecodeJson, [[ {"a":"b"}/ ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_object_trailing_comment_slash_open.json
assert(nil ~= pcall(DecodeJson, [[ {"a":"b"}// ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_object_trailing_comment_open.json
assert(nil ~= pcall(DecodeJson, [[ {"a":"b"}/**// ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_object_trailing_comment.json
assert(nil ~= pcall(DecodeJson, [[ {"a":"b"}/**/ ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_object_trailing_comma.json
assert(nil ~= pcall(DecodeJson, [[ {"id":0,} ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_object_several_trailing_commas.json
assert(nil ~= pcall(DecodeJson, [[ {"id":0,,,,,} ]]))

-- from fail2.lua

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_structure_trailing_#.json
assert(nil ~= pcall(DecodeJson, [[ {"a":"b"}#{} ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_structure_object_with_trailing_garbage.json
assert(nil ~= pcall(DecodeJson, [[ {"a": true} "x" ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_structure_object_followed_by_closing_object.json
assert(nil ~= pcall(DecodeJson, [[ {}} ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_structure_number_with_trailing_garbage.json
assert(nil ~= pcall(DecodeJson, [[ 2@ ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_structure_no_data.json
assert(nil ~= pcall(DecodeJson, [[  ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_structure_end_array.json
assert(nil ~= pcall(DecodeJson, [[ ] ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_structure_double_array.json
assert(nil ~= pcall(DecodeJson, [[ [][] ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_structure_close_unopened_array.json
assert(nil ~= pcall(DecodeJson, [[ 1] ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_structure_array_with_extra_array_close.json
-- (added spaces between [[ and ]] so lua doesn't get confused)
assert(nil ~= pcall(DecodeJson, [[
[ 1] ]   ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_structure_array_trailing_garbage.json
assert(nil ~= pcall(DecodeJson, [[ [1]x ]]))


-- from fail1.lua


-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_object_missing_semicolon.json
assert(nil ~= pcall(DecodeJson, [[ {"a" "b"} ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_object_double_colon.json
assert(nil ~= pcall(DecodeJson, [[ {"x"::"b"} ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_object_comma_instead_of_colon.json
assert(nil ~= pcall(DecodeJson, [[ {"x", null} ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_object_bad_value.json
assert(nil ~= pcall(DecodeJson, [[ ["x", truth] ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_number_with_leading_zero.json
assert(nil ~= pcall(DecodeJson, [[ [012] ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_number_real_without_fractional_part.json
assert(nil ~= pcall(DecodeJson, [[ [1.] ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_number_neg_int_starting_with_zero.json
assert(nil ~= pcall(DecodeJson, [[ [-012] ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_number_minus_space_1.json
assert(nil ~= pcall(DecodeJson, [[ [- 1] ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_number_2.e3.json
assert(nil ~= pcall(DecodeJson, [[ [2.e3] ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_number_2.e-3.json
assert(nil ~= pcall(DecodeJson, [[ [2.e-3] ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_number_2.e+3.json
assert(nil ~= pcall(DecodeJson, [[ [2.e+3] ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_number_1_000.json
assert(nil ~= pcall(DecodeJson, [[ [1 000.0] ]]))



-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_number_0.e1.json
assert(nil ~= pcall(DecodeJson, [[ [0.e1] ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_number_-2..json
assert(nil ~= pcall(DecodeJson, [[ [-2.] ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_number_-01.json
assert(nil ~= pcall(DecodeJson, [[ [-01] ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_multidigit_number_then_00.json
assert(nil ~= pcall(DecodeJson, [[ 123  ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_array_number_and_several_commas.json
assert(nil ~= pcall(DecodeJson, [[ [1,,] ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_array_number_and_comma.json
assert(nil ~= pcall(DecodeJson, [[ [1,] ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_array_missing_value.json
assert(nil ~= pcall(DecodeJson, [[ [   , ""] ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_array_just_minus.json
assert(nil ~= pcall(DecodeJson, [[ [-] ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_array_just_comma.json
assert(nil ~= pcall(DecodeJson, [[ [,] ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_array_inner_array_no_comma.json
-- (added spaces between [[ and ]] so lua doesn't get confused)
assert(nil ~= pcall(DecodeJson, [[
[ 3[ 4] ]   ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_array_extra_comma.json
assert(nil ~= pcall(DecodeJson, [[ ["",] ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_array_extra_close.json
-- (added spaces between [[ and ]] so lua doesn't get confused)
assert(nil ~= pcall(DecodeJson, [[
[ "x"] ]   ]]))



-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_array_double_extra_comma.json
assert(nil ~= pcall(DecodeJson, [[ ["x",,] ]]))


-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_array_double_comma.json
assert(nil ~= pcall(DecodeJson, [[ [1,,2] ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_array_comma_and_number.json
assert(nil ~= pcall(DecodeJson, [[ [,1] ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_array_comma_after_close.json
assert(nil ~= pcall(DecodeJson, [[ [""], ]]))

-- https://github.com/nst/JSONTestSuite/tree/d64aefb55228d9584d3e5b2433f720ea8fd00c82/test_parsing/n_array_1_true_without_comma.json
assert(nil ~= pcall(DecodeJson, [[ [1 true] ]]))
