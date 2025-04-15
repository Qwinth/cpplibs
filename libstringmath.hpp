// version 1.0
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
        UNARY_MINUS,
        POWER,
        MUL,
        DIV,
        PLUS,
        MINUS,
        NUMBER,
        UNDEFINED
    };

    std::map<TokenType, int> tokenPriority = {{EXPRESSION, 0}, {UNARY_MINUS, 1}, {POWER, 1}, {MUL, 2}, {DIV, 2}, {PLUS, 3}, {MINUS, 3}, {NUMBER, 4}};
    std::map<char, TokenType> mathOps = {{'+', PLUS}, {'-', MINUS}, {'*', MUL}, {'/', DIV}, {'^', POWER}};
    
    struct MathToken {
        TokenType type;
        std::string value;
        double num_value;

        MathToken() {};
        MathToken(std::string str, TokenType type) : value(str), type(type) {}
        MathToken(double num) : num_value(num), type(NUMBER) {}
    };

    using math_vars = std::map<std::string, double>;
    using math_tokens = std::vector<MathToken>;

    bool is_operator(MathToken token) {
        return token.type == UNARY_MINUS || token.type == POWER || token.type == MUL || token.type == DIV || token.type == PLUS || token.type == MINUS;
    }

    bool is_pow_seq(math_tokens& tokens, math_tokens::iterator it) {
        size_t power_count = 0;

        for (; it != tokens.end(); it++) {
            if (it->type != UNARY_MINUS && it->type != NUMBER && it->type != POWER) break;
            else if (it->type == NUMBER && (it + 1)->type == POWER) power_count++;
        }

        return power_count > 1;
    }

    math_tokens::iterator get_pow_seq_end(math_tokens& tokens, math_tokens::iterator it) {
        for (; it != tokens.end(); it++) 
        if (it->type != UNARY_MINUS && it->type != NUMBER && it->type != POWER) break;
    
        return it;
    }

    void __math_set_vars(math_tokens& tokens, math_vars& vars) {
        for (auto& i : tokens)
        if (vars.find(i.value) != vars.end()) i = vars.at(i.value);
    }

    double solve(std::string, std::map<std::string, double> = {});

    std::string __parse_number(std::string& string, size_t& pos) {
        std::string ret;

        for (; pos < string.size(); pos++) {
            if (!isdigit(string[pos]) && string[pos] != '.') break;

            ret += string[pos];
        }

        pos--;

        return ret;
    }

    std::string __parse_expression(std::string& string, size_t& pos) {
        std::string ret;
        size_t brackets = 0;

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

    std::string __parse_undefined(std::string& string, size_t& pos) {
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

        for (size_t i = 0; i < string.size(); i++) {
            char ch = string[i];

            if (isdigit(ch))
            tokens.push_back(std::stod(__parse_number(string, i)));

            else if (mathOps.find(ch) != mathOps.end())
            tokens.push_back({std::string(1, ch), mathOps.at(ch)});

            else if (ch == '(')
            tokens.push_back({__parse_expression(string, i), EXPRESSION});

            else tokens.push_back({__parse_undefined(string, i), UNDEFINED});
        }

        return tokens;
    }

    void __math_preprocess(math_tokens& tokens) {
        for (auto it = tokens.begin(); it != tokens.end(); it++) {

            if (it->type == MINUS && (it == tokens.begin() || is_operator(*(it - 1))))
            it->type = UNARY_MINUS;

            else if (is_pow_seq(tokens, it)) {
                auto it_end = get_pow_seq_end(tokens, it);
                std::reverse(it, it_end);

                if (it_end == tokens.end()) it_end--;

                for (; it != it_end; it++)
                if (it->type == NUMBER && (it + 1) != tokens.end() && (it + 1)->type == UNARY_MINUS)
                std::reverse(it, (it + 1)); 
            }
        }
    }

    double __math_calculate(math_tokens& tokens, int priority, math_vars vars) {
        if (priority == tokenPriority[NUMBER]) return tokens.front().num_value;

        for (auto it = tokens.begin(); it != tokens.end(); it++) {

            if (tokenPriority.at(it->type) == priority) {
                if (it->type == EXPRESSION)
                *it = solve(it->value, vars);

                else if (it->type == UNARY_MINUS && (it + 1)->type == NUMBER) {
                    *it = -(it + 1)->num_value;
                    tokens.erase(it + 1);
                }

                else if (it->type == UNARY_MINUS && (it + 1)->type == UNARY_MINUS) {
                    tokens.erase(it + 1);
                    it = tokens.erase(it) - 1;
                }

                else if (it->type == POWER) {
                    double op1 = (it + 1)->num_value;
                    double op2 = (it - 1)->num_value;
                    
                    tokens.erase(it + 1);
                    it = tokens.erase(it - 1);

                    *it = pow(op1, op2);
                }

                else if (it->type == MUL) {
                    double op1 = (it - 1)->num_value;
                    double op2 = (it + 1)->num_value;
                    
                    tokens.erase(it + 1);
                    it = tokens.erase(it - 1);

                    *it = op1 * op2;
                }

                else if (it->type == DIV) {
                    double op1 = (it - 1)->num_value;
                    double op2 = (it + 1)->num_value;
                    
                    tokens.erase(it + 1);
                    it = tokens.erase(it - 1);

                    *it = op1 / op2;
                }

                else if (it->type == PLUS) {
                    double op1 = (it - 1)->num_value;
                    double op2 = (it + 1)->num_value;
                    
                    tokens.erase(it + 1);
                    it = tokens.erase(it - 1);

                    *it = op1 + op2;
                }

                else if (it->type == MINUS) {
                    double op1 = (it - 1)->num_value;
                    double op2 = (it + 1)->num_value;
                    
                    tokens.erase(it + 1);
                    it = tokens.erase(it - 1);

                    *it = op1 - op2;
                }
            }
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