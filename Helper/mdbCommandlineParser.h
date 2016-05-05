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
     * �����н�����
     *
     * 1. ����ѡ����-��ʼ�����Դ�0����1�����߶��������ѡ����ֻ��Ϊһ���ַ���ѡ�����Ͳ���֮��������һ��
     *    �հ��ַ�������֮��������һ���հ��ַ������пհ��ַ��Ĳ������������Ű�Χ��
     * 2. ѡ��Ҳ������--��ʼ��ֻ���Դ�0����1�����߶��������ѡ��������Ϊ1��������ַ����м䲻�����пհ��ַ���
     *    ѡ�����Ͳ���֮��������һ���հ��ַ�������֮��������һ���հ��ַ������пհ��ַ��Ĳ������������Ű�Χ��
     *    ѡ�����ͺ͵�һ������֮��Ҳ������=����������=ǰ�������пհ��ַ���
     * 3. û�в�������-��ʼ��ѡ����Ժϲ������Ҹ�ѡ����������һ��ѡ����Դ��������磺tar -zcvf xxx.tgz xxx����
     * 4. �����н�������Ժ�
     *	  _opt_args_pairs[0]._firstΪ����������·������_opt_args_pairs[0]._secondΪ����Ĳ������顢���߿�
     *    ����������һ��ѡ��ʱ������������һ��ѡ��ʱ��_opt_args_pairs[_opt_args_pairs.length() - 1]._first
     *    Ϊ���һ��ѡ������_opt_args_pairs[_opt_args_pairs.length() - 1]._second��Ϊ���ѡ��Ĳ�������
     *    �ͳ���Ĳ�������֮�ͣ�����������ѡ�_opt_args_pairs[i]._firstΪѡ������_opt_args_pairs[i]._second
     *    Ϊ�������飻
     */
    class CommandLineParser
    {
    public:
    	struct OptArgsPair
    	{
    		OptArgsPair(const string& first, const vector<string>& second) : _first(first), _second(second) {}
    		bool operator ==(const OptArgsPair& rhs) const { return _first == rhs._first; }		// ע�⣺����ȫ���

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
    		bool operator==(const Condition& rhs) const { return _opt == rhs._opt; }			// ע�⣺����ȫ���
    		
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
