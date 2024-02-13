#include <iostream>
#include <fstream>
#include <sstream>

#include <vector>
#include <algorithm>
#include <iterator>
#include <iomanip>
#include <regex>
#include <chrono>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

using namespace std;
using namespace nlohmann;


const string github_token = "";
const string repo = "Hxnter999/ThunderDumps";
const vector<string> keywords = { "playerlist", "cgame", "localplayer", "chud" };
const int offsets_num = keywords.size();



//A
template <typename T>
ostream& operator<<(ostream& os, vector<T> vec)
{
    if constexpr (is_same<T, byte>::value)
    {
        os << hex << noshowbase << uppercase;
        for (int i = 0; i < vec.size(); i++)
        {
            os << setfill('0') << setw(2) << (int)(vec[i]) << " ";
        }
        os << nouppercase << dec;
    }
    else
    {
        for (int i = 0; i < vec.size(); i++)
        {
            os << vec[i] << endl;
        }
    }
    return os;
}


void log_error(string msg, int code) 
{
    string filename = "error_log.txt";
    ofstream error_log(filename, ios_base::app);
    if (!error_log) 
    {
        cerr << "[-] Failed to open error log file " << filename << endl;
    }
    else 
    {
        time_t now = time(0);
        tm localtm;
        localtime_s(&localtm, &now);
        char timeStr[100];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &localtm);
        error_log << "[" << timeStr << "]\t\t" << msg << endl;
    }
    cerr << "[-] " << msg << endl;
    system("pause");
    exit(code);
}


vector<byte> int_to_le(int value)
{
    // Converts integer to 4 bytes in little endian (LE)
    int bytes_num = 4;
    vector<byte> bytes = {};
    for (int i = 0; i < bytes_num; i++)
    {
        byte byte = (int)((value >> (i * 8)) & 0xFF);
        bytes.push_back(byte);
    }
    return bytes;
}


class Offset
{
public:
    string name;
    int position;
    int old_value;
    int new_value;
    vector<byte> old_bytes;
    vector<byte> new_bytes;


    Offset()
    {
        this->name = "";
        this->position = 0;
        this->old_value = 0;
        this->new_value = 0;
        this->old_bytes = {};
        this->new_bytes = {};
    }


    Offset(string name, int position, int old_value, int new_value)
    {
        this->name = name;
        this->position = position;
        this->old_value = old_value;
        this->new_value = new_value;
        this->old_bytes = int_to_le(old_value);
        this->new_bytes = int_to_le(new_value);
    }


    void status()
    {
        uintptr_t address = 0x140001000 + this->position - 0x400;

        cout << "Offset: " << this->name << endl;
        cout << "Position: " << hex << showbase << this->position << "    [ " << address << " ]" << dec << endl;
        cout << "Old: " << hex << showbase << setfill(' ') << setw(10) << this->old_value << dec;
        cout << " | " << this->old_bytes << endl;
        cout << "New: " << hex << showbase << setfill(' ') << setw(10) << this->new_value << dec;
        cout << " | " << this->new_bytes << endl;
        cout << endl;
    }
};


//B
size_t WriteCallback(char* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


string make_request(string url)
{
    CURL* curl = curl_easy_init();
    string response;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "User-Agent: DS-Overlay AutoPatcher");
    if (!github_token.empty())
    {
        headers = curl_slist_append(headers, ("Authorization: " + github_token).c_str());
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    CURLcode code = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (code != CURLE_OK)
    {
        string msg = "curl_easy_perform() failed: " + (string)curl_easy_strerror(code);
        log_error(msg, 1);
    }

    if (response.find("API rate limit exceeded") != string::npos)
    {
        log_error(response, 2);
    }
    return response;
}


vector<string> get_versions()
{
    string url = "https://api.github.com/repos/" + repo + "/git/trees/main?recursive=1";
    string response = make_request(url);
    json dirs = json::parse(response);

    vector<string> versions;
    for (auto& dir : dirs["tree"])
    {
        if (dir["type"] == "tree" && dir["path"].get<string>().find("Versions/") == 0)
        {
            string version = dir["path"].get<string>().substr(9);  // Remove "Versions/" from the path
            versions.push_back(version);
        }
    }
    return versions;
}


vector<int> split(string str, char delimiter)
{
    vector<int> tokens;
    istringstream istrstream(str);
    string token;
    while (getline(istrstream, token, delimiter))
    {
        try
        {
            tokens.push_back(stoi(token));
        }
        catch (exception e)
        {
            cerr << "Caught exception: " << e.what() << endl;
        }
    }
    return tokens;
}


bool cmp_versions(string version1, string version2)
{
    vector<int> parts1 = split(version1, '.');
    vector<int> parts2 = split(version2, '.');

    for (int i = 0; i < parts1.size() && i < parts2.size(); i++) {
        if (parts1[i] < parts2[i]) return true;
        if (parts1[i] > parts2[i]) return false;
    }

    return parts1.size() < parts2.size();
}


void sort_versions(vector<string>& versions)
{
    sort(versions.begin(), versions.end(), cmp_versions);
    reverse(versions.begin(), versions.end());

    // Select only versions newer than 2.31.1.50
    string truncate_after = "2.31.1.50"; // The first version when offsets.txt appears
    auto it = find(versions.begin(), versions.end(), truncate_after);
    if (it != versions.end())
    {
        ++it;
        versions.erase(it, versions.end());
    }
}


// C
fstream open_file(string filename)
{
    fstream file(filename, ios::binary | ios::in | ios::out);
    if (!file) 
    {
        log_error("Failed to open file " + filename, 3);
    }
    return file;
}


bool file_exists(string filename)
{
    ifstream file(filename);
    return file.good();
}


void copy_file(string source, string destination)
{
    ifstream src(source, ios::binary);
    ofstream dst(destination, ios::binary);
    dst << src.rdbuf();
    src.close();
    dst.close();
}


bool cmp_files(string filename1, string filename2)
{
    cout << "Comparing " + filename1 + " & " + filename2 + ":\n";

    fstream file1 = open_file(filename1);
    fstream file2 = open_file(filename2);

    vector<unsigned char> file_data1((istreambuf_iterator<char>(file1)), istreambuf_iterator<char>());
    vector<unsigned char> file_data2((istreambuf_iterator<char>(file2)), istreambuf_iterator<char>());

    if (file_data1.size() != file_data2.size())
    {
        cout << "Files have different sizes\n";
        return false;
    }

    for (int i = 0; i < file_data1.size(); i++)
    {
        if (file_data1[i] != file_data2[i])
        {
            cout << "Index of the differing char = " << i << endl;
            return false;
        }
    }

    cout << "Files are identical\n";
    file1.close();
    file2.close();
    return true;
}


// D
string get_keyword(string name)
{
    for (const auto& keyword : keywords)
    {
        if (name.find(keyword) != string::npos)
        {
            return keyword;
        }
    }
    return "";
}


int find_position(string filename, vector<byte> bytes)
{
    // Finds a position of 1st byte of offset
    fstream file = open_file(filename);
    vector<byte> data((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    auto iterator = search(data.begin(), data.end(), bytes.begin(), bytes.end());
    int position = -1;
    if (iterator != data.end())
    {
        position = distance(data.begin(), iterator);
    }

    file.close();
    return position;
}


void replace_offset(string filename, int position, vector<byte> bytes)
{
    fstream file = open_file(filename);
    file.seekp(position);
    for (char byte : bytes)
    {
        file.write(&byte, 1);
    }
    file.close();
}


// E
bool get_offsets(vector<shared_ptr<Offset>>& offsets, string version, bool old_version)
{
    string url = "https://raw.githubusercontent.com/" + repo + "/main/Versions/" + version + "/offsets.txt";
    string response = make_request(url);
    for (auto& c : response)
    {
        c = tolower(c);
    }

    // Return if version dir doesn't contain offsets.txt
    if (response.find("404: not found") != string::npos)
    {
        return false;
    }

    // Read offsets.txt line by line. Parse name of the offset and its value
    map<string, int> name_value;
    istringstream istrstream(response);
    string line;
    while (getline(istrstream, line))
    {
        regex pattern_regex(R"(constexpr\s+uintptr_t\s+(\w+)\s*=\s*(0x[0-9a-f]+);)");
        smatch pattern_match;
        if (!regex_match(line, pattern_match, pattern_regex))
        {
            continue;
        }

        int value = stoul(pattern_match[2], nullptr, 16);
        string name = get_keyword(pattern_match[1]);
        if (name == "")
        {
            continue;
        }

        // Add name-value pair to map
        name_value[name] = value;
    }

    if (name_value.size() != offsets_num)
    {
        return false;
    }

    // Update offsets with new/old values
    for (auto& offset : offsets)
    {
        auto it = name_value.find(offset->name);
        if (it != name_value.end())
        {
            if (old_version == true)
            {
                offset->old_value = it->second;
                offset->old_bytes = int_to_le(it->second);
            }
            else
            {
                offset->new_value = it->second;
                offset->new_bytes = int_to_le(it->second);
            }
        }
    }

    return true;
}


bool find_positions(vector<shared_ptr<Offset>>& offsets, string filename)
{
    for (auto& offset : offsets)
    {
        int position = find_position(filename, offset->old_bytes);
        if (position == -1)
        {
            return false;
        }
        offset->position = position;
    }
    return true;
}


void replace_offsets(vector<shared_ptr<Offset>> offsets, string filename)
{
    for (auto& offset : offsets)
    {
        replace_offset(filename, offset->position, offset->new_bytes);
    }
}


// F
void update_offsets(vector<string> versions, string filename)
{
    for (int i = 0; i < versions.size(); i++)
    {
        cout << "Checking version: " << versions[i] << endl << endl;

        vector<shared_ptr<Offset>> offsets = {};
        for (const auto& keyword : keywords)
        {
            shared_ptr<Offset> offset = make_shared<Offset>(keyword, 0, 0, 0);
            offsets.push_back(offset);
        }

        // Obtain old offsets from github
        if (get_offsets(offsets, versions[i], true) == false)
        {
            continue;
        }

        // Find positions of old offsets in .exe
        if (find_positions(offsets, filename) == false)
        {
            continue;
        }

        cout << "[+] Current version: " << versions[i] << endl;
        cout << "[?] Latest version: " << versions[0] << endl << endl;

        // Obtain new offsets from github
        if (get_offsets(offsets, versions[0], false) == false)
        {
            log_error("Cannot obtain new offsets from latest version " + versions[0], 4);
        }

        for (const auto& offset : offsets)
        {
            offset->status();
        }

        // Patch .exe with new offsets
        replace_offsets(offsets, filename);
        cout << "[+] Updated" << endl;
        return;
    }
    log_error("Cannot detect a version of " + filename, 5);
}


int main(int argc, char* argv[])
{
    string old_filename = "old.exe";
    string new_filename = "new.exe";

    if (file_exists(old_filename) == false)
    {
        string msg = "File " + old_filename + " doesn't exist. Please, copy-paste it or check file permissions.";
        log_error(msg, 6);
    }

    //cmp_files("new.exe", "updated.exe");
    //exit(33);

    copy_file(old_filename, new_filename);
    vector<string> versions = get_versions();

    //vector<string> versions = {
    //"2.33.0.111",
    //"2.25.1.89",
    //"2.27.2.34",
    //"2.25.1.92",
    //"2.33.0.114", // old
    //"2.27.0.34",
    //"2.31.1.50",
    //"2.27.2.22",
    //"2.33.0.110",
    //"2.27.2.25",
    //"2.33.0.117", // new
    //"2.27.0.29",
    //};

    sort_versions(versions);
    update_offsets(versions, new_filename);

    system("pause");
    return 0;
}