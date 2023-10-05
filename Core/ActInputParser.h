#ifndef ActInputParser_h
#define ActInputParser_h
/*
Manager of configuration files
*/
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
namespace ActRoot
{
    const std::string kTokenSeparator {": "};
    const std::string kValueSeparator {", "};
    const std::string kCommentSeparator {"%"};
    const std::string kBlockOpening {"["};
    const std::string kBlockClosing {"]"};
    const std::string kExpandValue {"..."};
    
    std::string StripSpaces(std::string line);
    
    class InputBlock
    {
    private:
        std::string fBlockName;
        std::vector<std::string> fLines;
        std::vector<std::string> fTokens;
        std::unordered_map<std::string, std::vector<std::string>> fValues;
    public:
        InputBlock(){};
        InputBlock(const std::string& name){ fBlockName = name; };
        ~InputBlock(){};

        void AddLine(const std::string& line);
        std::string GetBlockName() const {return fBlockName;}
        bool CheckTokenExists(const std::string& token, bool soft = false);
        std::string GetString(const std::string& token);
        int GetInt(const std::string& token);
        bool GetBool(const std::string& token);
        double GetDouble(const std::string& token);
        std::vector<std::string> GetStringVector(const std::string& token);
        std::vector<int> GetIntVector(const std::string& token);
        std::vector<bool> GetBoolVector(const std::string& token);
        std::vector<double> GetDoubleVector(const std::string& token);
        std::unordered_map<std::string, std::vector<std::string>> GetAllReadValues() const {return fValues;}
        std::vector<std::string> GetTokens() const {return fTokens;}
    private:
        std::string GetToken(const std::string& line);
        void GetValues(const std::string& line, const std::string& token, bool findTokenSeparator = true);
        bool IsVector(const std::string& token);
        int StringToInt(const std::string& val);
        bool StringToBool(const std::string& val);
        double StringToDouble(const std::string& val);
        std::vector<int> ExpandInt(int begin, int end);
    };

    typedef std::shared_ptr<InputBlock> BlockPtr;
    class InputParser
    {
    private:
        std::vector<BlockPtr> fBlocks;
    public:
        InputParser(){};
        InputParser(const std::string& filename){ ReadFile(filename); };

        void ReadFile(const std::string& filename);
        void Print() const;
        BlockPtr GetBlock(const std::string& header) const;
        std::vector<std::string> GetBlockHeaders() const;
    private:
        bool IsComment(const std::string& line);
        std::string IsBlockHeader(const std::string& line);
    };
}
#endif
