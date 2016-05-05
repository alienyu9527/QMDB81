#if !defined(COMMAND_LINE_PARSER_H__8234782388489238948932894234324892389489893)
#define COMMAND_LINE_PARSER_H__8234782388489238948932894234324892389489893

#ifdef WIN32
#pragma warning(disable: 4786)
#endif // #ifdef WIN32

#include <string>
#include <vector>
#include <iostream>
using namespace std;

//namespace QuickMDB{
        

    /*
     * 命令行解析器
     *
     * 1. 所有选项以-开始，可以带0个、1个或者多个参数；选项名只能为一个字符；选项名和参数之间至少有一个
     *    空白字符；参数之间至少有一个空白字符；含有空白字符的参数必须用引号包围；
     * 2. 选项也可以以--开始，只可以带0个、1个或者多个参数；选项名可以为1个到多个字符，中间不允许有空白字符；
     *    选项名和参数之间至少有一个空白字符；参数之间至少有一个空白字符；含有空白字符的参数必须用引号包围；
     *    选项名和和第一个参数之间也可以用=隔开，并且=前后不允许有空白字符；
     * 3. 没有参数的以-开始的选项可以合并，并且该选项组合中最后一个选项可以带参数（如：tar -zcvf xxx.tgz xxx）；
     * 4. 命令行解析完毕以后：
     *	  _opt_args_pairs[0]._first为程序名（含路径），_opt_args_pairs[0]._second为程序的参数数组、或者空
     *    串（含至少一个选项时）；在至少有一个选项时，_opt_args_pairs[_opt_args_pairs.length() - 1]._first
     *    为最后一个选项名，_opt_args_pairs[_opt_args_pairs.length() - 1]._second则为这个选项的参数数组
     *    和程序的参数数组之和；对于其他的选项，_opt_args_pairs[i]._first为选项名，_opt_args_pairs[i]._second
     *    为参数数组；
     */
    class CommandLineParser
    {
    public:
    	struct OptArgsPair
    	{
    		OptArgsPair(const string& first, const vector<string>& second) : _first(first), _second(second) {}
    		bool operator ==(const OptArgsPair& rhs) const { return _first == rhs._first; }		// 注意：非完全相等

    		string _first;
    		vector<string> _second;
    	};
    	
    private:
    	struct Condition
    	{
    		Condition(const string& opt, size_t arg_count)
    			: _opt(opt), _arg_count_min(arg_count), _arg_count_max(arg_count)
    		{
    		}
    		Condition(const string& opt, size_t arg_count_min, size_t arg_count_max)
    			: _opt(opt), _arg_count_min(arg_count_min), _arg_count_max(arg_count_max)
    		{
    			arg_count_max = arg_count_min > arg_count_max ? arg_count_min : arg_count_max;
    		}
    		bool operator==(const Condition& rhs) const { return _opt == rhs._opt; }			// 注意：非完全相等
    		
    		string _opt;
    		size_t _arg_count_min;
    		size_t _arg_count_max;
    	};

    protected:
    	static void print_check_error(const string& msg) { cout << "**** Command Line Error: " << msg << endl; }

    public:
    	CommandLineParser(int argc, char* argv[]) : _argc(argc), _argv(argv) { do_parse(); }
    	virtual ~CommandLineParser() {}

    	void set_check_condition(const string& opt, size_t arg_count);
    	void set_check_condition(const string& opt, size_t arg_count_min, size_t arg_count_max);
    	virtual bool check(void (*print_error_func)(const string& msg) = print_check_error);
    	
    	const vector<OptArgsPair>& opt_args_pairs() const { return _opt_args_pairs; }

    protected:
    	void do_parse();
    	
    protected:
    	int _argc;
    	char** _argv;
    	vector<OptArgsPair> _opt_args_pairs;
    	vector<Condition> _conditions;
    };

//}

#endif // #if !defined(COMMAND_LINE_PARSER_H__8234782388489238948932894234324892389489893)
