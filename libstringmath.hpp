// version 1.1-c1
#pragma once
#include <string>
#include <cmath>
#include <map>
#include <vector>
#include <algorithm>
#include "libstrmanip.hpp"

namespace string_math {
    enum TokenType {
        EXPRESSION,
        POWER,
        UNARY_MINUS,
        MUL,
        DIV,
        PLUS,
        MINUS,
        NUMBER,
        UNDEFINED
    };

    std::map<TokenType, int> tokenPriority = {{EXPRESSION, 0}, {POWER, 1}, {UNARY_MINUS, 1}, {MUL, 3}, {DIV, 3}, {PLUS, 4}, {MINUS, 4}, {NUMBER, 5}};
    std::map<char, TokenType> mathOps = {{'+', PLUS}, {'-', MINUS}, {'*', MUL}, {'/', DIV}, {'^', POWER}};
    
    struct MathToken {
        TokenType type;
        std::string value;
        double num_value;

        bool pow_seq;

        MathToken() {};
        MathToken(std::string str, TokenType type) : value(str), type(type) {}
        MathToken(double num) : num_value(num), type(NUMBER) {}
    };

    using math_vars = std::map<std::string, double>;
    using math_tokens = std::vector<MathToken>;

    // void printTokens(math_tokens tokens) {
    //     for (auto i : tokens) {
    //         std::cout << "token type: " << i.type << " value: " << ((i.type == NUMBER) ? std::to_string(i.num_value) : i.value) << std::endl;
    //     }
    // }

    bool is_operator(MathToken token) {
        return token.type == UNARY_MINUS || token.type == POWER || token.type == MUL || token.type == DIV || token.type == PLUS || token.type == MINUS;
    }

    bool is_pow_exp(math_tokens& tokens, int64_t i, bool invert) {
        if (i + 2 >= tokens.size()) return false;
        else if (invert && i - 2 < 0) return false;

        if (invert) return tokens[i].type == NUMBER && tokens[i - 1].type == POWER;

        return tokens[i].type == NUMBER && tokens[i + 1].type == POWER;
    }

    bool is_pow_seq(math_tokens& tokens, int64_t i) {
        int64_t power_count = 0;

        for (; i < tokens.size(); i++) {
            if (tokens[i].type != UNARY_MINUS && tokens[i].type != NUMBER && tokens[i].type != POWER && tokens[i].type != EXPRESSION) break;
            else if ((tokens[i].type == NUMBER || tokens[i].type == EXPRESSION) && tokens[i + 1].type == POWER) power_count++;
        }

        return power_count > 1;
    }

    int64_t get_pow_seq_end(math_tokens& tokens, int64_t i) {
        for (; i < tokens.size(); i++)
        if (tokens[i].type != UNARY_MINUS && tokens[i].type != NUMBER && tokens[i].type != POWER && tokens[i].type != EXPRESSION) break;

        return i;
    }

    void __math_set_vars(math_tokens& tokens, math_vars& vars) {
        for (auto& i : tokens) if (vars.find(i.value) != vars.end()) i = vars.at(i.value);
    }

    double solve(std::string, std::map<std::string, double> = {{"p", M_PI}, {"e", M_E}});

    std::string __parse_number(std::string& string, int64_t& pos) {
        std::string ret;

        for (; pos < string.size(); pos++) {
            if (!isdigit(string[pos]) && string[pos] != '.') break;

            ret += string[pos];
        }

        pos--;

        return ret;
    }

    std::string __parse_expression(std::string& string, int64_t& pos) {
        std::string ret;
        int64_t brackets = 0;

        for (; pos < string.size(); pos++) {
            char ch = string[pos];

            if (ch == '(') brackets++;
            else if (ch == ')') brackets--;

            if (ch == '(' && brackets == 1) continue;
            if (!brackets) break;

            ret += ch;
        }

        return ret;
    }

    std::string __parse_undefined(std::string& string, int64_t& pos) {
        std::string ret;

        for (; pos < string.size() && isalpha(string[pos]); pos++) ret += string[pos];
        pos--;

        return ret;
    }

    math_tokens __math_parse(std::string& string) {
        replaceAll(string, " ", "");
        replaceAll(string, "\r", "");
        replaceAll(string, "\n", "");
        replaceAll(string, "\t", "");

        math_tokens tokens;

        for (int64_t i = 0; i < string.size(); i++) {
            char ch = string[i];

            if (isdigit(ch)) tokens.push_back(std::stod(__parse_number(string, i)));

            else if (mathOps.find(ch) != mathOps.end()) tokens.push_back({std::string(1, ch), mathOps.at(ch)});

            else if (ch == '(') tokens.push_back({__parse_expression(string, i), EXPRESSION});

            else tokens.push_back({__parse_undefined(string, i), UNDEFINED});
        }

        return tokens;
    }

    void __math_preprocess(math_tokens& tokens) {
        for (int64_t i = 0; i < tokens.size(); i++)
        if (tokens[i].type == MINUS && (!i || is_operator(tokens[i - 1]))) tokens[i].type = UNARY_MINUS;            

        for (int64_t i = 0; i < tokens.size(); i++) {         
            if (tokens[i].type == POWER) tokens[i].pow_seq = false;

            else if (is_pow_seq(tokens, i)) {
                auto i_end = get_pow_seq_end(tokens, i);

                std::reverse(tokens.begin() + i, tokens.begin() + i_end);

                for (; i != i_end; i++) if (tokens[i].type == POWER) tokens[i].pow_seq = true;
            }
        }
    }

    double __math_calculate(math_tokens& tokens, int priority, math_vars vars) {
        if (!tokens.size()) return 0;
        if (priority == tokenPriority[NUMBER]) return tokens.front().num_value;

        int64_t curPriorOps = 0;

        for (int64_t i = 0; i < tokens.size(); i++)
        if (tokenPriority.at(tokens[i].type) == priority) curPriorOps++;

        for (int64_t i = 0; i < tokens.size(); i++) {
            
            if (tokenPriority.at(tokens[i].type) == priority) {
                if (tokens[i].type == EXPRESSION) {
                    tokens[i] = solve(tokens[i].value, vars);

                    curPriorOps--;
                }

                else if (tokens[i].type == UNARY_MINUS && tokens[i + 1].type == NUMBER && !is_pow_exp(tokens, i + 1, false)) {
                    tokens[i] = -tokens[i + 1].num_value;
                    tokens.erase(tokens.begin() + i + 1);

                    curPriorOps--;
                }

                else if (tokens[i].type == UNARY_MINUS && tokens[i - 1].type == NUMBER && !is_pow_exp(tokens, i - 1, true)) {
                    tokens[i] = -tokens[i - 1].num_value;
                    tokens.erase(tokens.begin() + i - 1);

                    i--;
                    curPriorOps--;
                }

                else if (tokens[i].type == UNARY_MINUS && tokens[i + 1].type == UNARY_MINUS) {
                    tokens.erase(tokens.begin() + i + 1);
                    tokens.erase(tokens.begin() + i);

                    i--;
                    curPriorOps -= 2;
                }

                else if (tokens[i].type == POWER && tokens[i - 1].type == NUMBER && tokens[i + 1].type == NUMBER) {
                    double op1 = tokens[i - 1].num_value;
                    double op2 = tokens[i + 1].num_value;

                    if (tokens[i].pow_seq) {
                        op1 = tokens[i + 1].num_value;
                        op2 = tokens[i - 1].num_value;
                    }
                    
                    tokens.erase(tokens.begin() + i + 1);
                    tokens.erase(tokens.begin() + i - 1);

                    i--;
                    curPriorOps--;

                    tokens[i] = pow(op1, op2);
                }

                else if (tokens[i].type == MUL && tokens[i - 1].type == NUMBER && tokens[i + 1].type == NUMBER) {
                    double op1 = tokens[i - 1].num_value;
                    double op2 = tokens[i + 1].num_value;
                    
                    tokens.erase(tokens.begin() + i + 1);
                    tokens.erase(tokens.begin() + i - 1);

                    i--;
                    curPriorOps--;

                    tokens[i] = op1 * op2;
                }

                else if (tokens[i].type == DIV && tokens[i - 1].type == NUMBER && tokens[i + 1].type == NUMBER) {
                    double op1 = tokens[i - 1].num_value;
                    double op2 = tokens[i + 1].num_value;
                    
                    tokens.erase(tokens.begin() + i + 1);
                    tokens.erase(tokens.begin() + i - 1);

                    i--;
                    curPriorOps--;

                    tokens[i] = op1 / op2;
                }

                else if (tokens[i].type == PLUS && tokens[i - 1].type == NUMBER && tokens[i + 1].type == NUMBER) {
                    double op1 = tokens[i - 1].num_value;
                    double op2 = tokens[i + 1].num_value;
                    
                    tokens.erase(tokens.begin() + i + 1);
                    tokens.erase(tokens.begin() + i - 1);

                    i--;
                    curPriorOps--;

                    tokens[i] = op1 + op2;
                }

                else if (tokens[i].type == MINUS && tokens[i - 1].type == NUMBER && tokens[i + 1].type == NUMBER) {
                    double op1 = tokens[i - 1].num_value;
                    double op2 = tokens[i + 1].num_value;
                    
                    tokens.erase(tokens.begin() + i + 1);
                    tokens.erase(tokens.begin() + i - 1);

                    i--;
                    curPriorOps--;

                    tokens[i] = op1 - op2;
                }
            }

            if (i + 1 >= tokens.size() && curPriorOps) i = -1;
        }

        return __math_calculate(tokens, priority + 1, vars);
    }

    double solve(std::string string, math_vars vars) {
        math_tokens tokens = __math_parse(string);

        __math_set_vars(tokens, vars);
        __math_preprocess(tokens);
        
        return __math_calculate(tokens, 0, vars);
    }
}