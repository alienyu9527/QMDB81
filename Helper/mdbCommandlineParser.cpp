#ifdef WIN32
#pragma warning(disable: 4786)
#endif // #ifdef WIN32

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "string_ex.h"
#include "mdbCommandlineParser.h"
using namespace std;


//namespace QuickMDB{
        
    void CommandLineParser::set_check_condition(const string& opt, size_t arg_count)
    {
    	if (opt.empty())
    	{
    		_conditions.push_back(Condition(_argv[0], arg_count));
    		return;
    	}
    	_conditions.push_back(Condition(opt, arg_count));
    }

    void CommandLineParser::set_check_condition(const string& opt, size_t arg_count_min, size_t arg_count_max)
    {
    	if (opt.empty())
    	{
    		_conditions.push_back(Condition(_argv[0], arg_count_min, arg_count_max));
    		return;
    	}
    	_conditions.push_back(Condition(opt, arg_count_min, arg_count_max));
    }

    bool CommandLineParser::check(void (*print_error_func)(const string& msg) /*= print_check_error*/)
    {
    	vector<OptArgsPair>::iterator it = find(_opt_args_pairs.begin(), _opt_args_pairs.end(), OptArgsPair("-", vector<string>()));
    	if (it != _opt_args_pairs.end())
    	{
    		if (print_check_error != NULL)
    		{
    			print_check_error("no option name followed -");
    		}
    		return false;
    	}
    	
    	it = find(_opt_args_pairs.begin(), _opt_args_pairs.end(), OptArgsPair("--", vector<string>()));
    	if (it != _opt_args_pairs.end())
    	{
    		if (print_check_error != NULL)
    		{
    			print_check_error("no option name followed --");
    		}
    		return false;
    	}

    	/*
    	 * 为最后一个选项保留最大数目个参数，其余的参数移动到程序的参数数组
    	 */
    	do {
    		if (_opt_args_pairs.size() > 1)
    		{
    			OptArgsPair& last_pair = _opt_args_pairs[_opt_args_pairs.size() - 1];
    			vector<Condition>::iterator it_cond = find(_conditions.begin(), _conditions.end(), Condition(last_pair._first, 0));
    			if (it_cond == _conditions.end())
    			{
    				break;
    			}

    			if ((*it_cond)._arg_count_max >= last_pair._second.size())
    			{
    				break;
    			}
    			size_t i;
    			for (i = (*it_cond)._arg_count_max; i < last_pair._second.size(); ++i)
    			{
    				_opt_args_pairs[0]._second.push_back(last_pair._second[i]);
    			}
    			last_pair._second.erase(last_pair._second.begin() + (*it_cond)._arg_count_max, last_pair._second.end());

    		}
    	} while(false);

    	for (it = _opt_args_pairs.begin(); it != _opt_args_pairs.end(); ++it)
    	{
    		/*
    		 * 单独处理程序及其参数
    		 */
    		if (it == _opt_args_pairs.begin())
    		{
    			vector<Condition>::iterator it_cond = find(_conditions.begin(), _conditions.end(), Condition((*it)._first, 0));
    			if (it_cond == _conditions.end())
    			{
    				continue;
    			}
    			if ((*it)._second.size() < (*it_cond)._arg_count_min || (*it)._second.size() > (*it_cond)._arg_count_max)
    			{
    				if (print_check_error != NULL)
    				{
    					string_ex msg;
    					if ((*it_cond)._arg_count_min == (*it_cond)._arg_count_max)
    					{
    						msg.format("the program can only has %d argument", 
    							(*it_cond)._arg_count_min
    						);
    					}
    					else
    					{
    						msg.format("the program can only has %d~%d argument(s)", 
    							(*it_cond)._arg_count_min,
    							(*it_cond)._arg_count_max
    						);
    					}
    					print_check_error(msg);
    				}
    				return false;
    			}
    			continue;
    		}

    		/*
    		 * 处理各选项及其参数
    		 */
    		vector<Condition>::iterator it_cond = find(_conditions.begin(), _conditions.end(), Condition((*it)._first, 0));
    		if (it_cond == _conditions.end())
    		{
    			string_ex msg;
    			msg.format("unknown option name: %s", (*it)._first.c_str());
    			if (print_check_error != NULL)
    			{
    				print_check_error(msg);
    			}
    			return false;
    		}
    		if ((*it)._second.size() < (*it_cond)._arg_count_min || (*it)._second.size() > (*it_cond)._arg_count_max)
    		{
    			string_ex msg;
    			if ((*it_cond)._arg_count_min == (*it_cond)._arg_count_max)
    			{
    				msg.format("option %s can only has %d argument",
    					(*it)._first.c_str(),
    					(*it_cond)._arg_count_min
    				);
    			}
    			else
    			{
    				msg.format("option %s can only has %d~%d argument(s)",
    					(*it)._first.c_str(),
    					(*it_cond)._arg_count_min,
    					(*it_cond)._arg_count_max
    				);
    			}
    			if (print_check_error != NULL)
    			{
    				print_check_error(msg);
    			}
    			return false;
    		}
    	}

    	return true;
    }

    void CommandLineParser::do_parse()
    {
    	int i;
    	for (i = 0; i < _argc; ++i)
    	{
    		string_ex cur_val(_argv[i]);
    		
    		if (i == 0)											// the executable name
    		{
    			_opt_args_pairs.clear();
    			OptArgsPair pair(cur_val, vector<string>());
    			_opt_args_pairs.push_back(pair);
    			continue;
    		}

    		if (cur_val.left(2) == string_ex("--"))				// an option name with a prefix --
    		{
    			if (cur_val.length() == 2)
    			{
    				OptArgsPair pair("--", vector<string>());
    				_opt_args_pairs.push_back(pair);
    				continue;
    			}
    			
    			size_t pos = cur_val.find("=");
    			if (pos == string_ex::npos)
    			{
    				OptArgsPair pair(cur_val, vector<string>());
    				_opt_args_pairs.push_back(pair);
    				continue;
    			}
    			string_ex opt(cur_val.c_str(), pos);
    			string_ex arg(cur_val.c_str() + pos + 1);
    			vector<string> second;
    			second.push_back(arg);
    			OptArgsPair pair(opt, second);
    			_opt_args_pairs.push_back(pair);
    			continue;
    		}

    		if (cur_val.left(1) == string_ex("-"))				// an option name with a prefix -
    		{
    			if (cur_val.length() == 1)
    			{
    				OptArgsPair pair("-", vector<string>());
    				_opt_args_pairs.push_back(pair);
    				continue;
    			}
    			
    			size_t i;
    			for (i = 1; i < cur_val.length(); ++i)
    			{
    				string_ex opt = string_ex("-") + cur_val[i];
    				OptArgsPair pair(opt, vector<string>());
    				_opt_args_pairs.push_back(pair);
    			}
    			continue;
    		}
    		
    		vector<string>& last_second = _opt_args_pairs[_opt_args_pairs.size() - 1]._second;
    		last_second.push_back(cur_val);
    		continue;
    	}
    }




//}












