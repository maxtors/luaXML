#include <fstream>
#include <string>
#include <map>
#include <vector>

extern "C" {
#include "lualib/lua.h"
#include "lualib/lualib.h"
#include "lualib/lauxlib.h"
}

// ---------- STRUCTS ---------------------------------------------------------
struct Tag {
    std::string name;                               // Tag ID
    std::map<std::string, std::string> attributes;  // Map of attributes
    std::vector<Tag*> tags;                         // Sub tags
    std::string tag_data;                           // String of tag data
};

struct File {
    std::string fdata;                              // The XML file data
    unsigned int pstart, pend;                      // Current positions
};

// ---------- FUNCTION DECLARATION --------------------------------------------
static int parseXML(lua_State *L);
void deleteXMLtags(Tag* t);
Tag* parseXMLtag(File* f, std::string main_name, bool &sub_ended);
std::string parseXMLname(std::string s);
std::string getXMLtag(File* f);
std::string parseXMLtagdata(File* f);
std::map<std::string, std::string> parseXMLattributes(std::string s);
void makeLUAtable(Tag* t);

// ---------- GLOBAL VARIABLES ------------------------------------------------
lua_State *L;

// ---------- MAIN ------------------------------------------------------------
int main() {
    L = lua_open();
    luaL_openlibs(L);
    lua_register(L, "parseXML", parseXML);
    luaL_dofile(L, "luaXML.lua");
    lua_close(L);
    system("pause");
    return 0;
}

// ---------- PARSE AN XML DOCUMENT -------------------------------------------
static int parseXML(lua_State *L) {
    File file;
    std::vector<Tag*> tags;
    Tag* root_tag;
    bool dummy;
    std::string root_name, filename, data;
    unsigned int size, start, end;
    std::ifstream inn;
    char* buffer;

    filename = luaL_checkstring(L, -1);         // Open wanted file and check
    inn.open(filename.c_str());                 // If it is open and good
    if (!inn) {lua_pushnil(L);return 1;}

    inn.seekg(0, std::ios::beg);                // Get the file size
    start = inn.tellg();
    inn.seekg(0, std::ios::end);
    end = inn.tellg();
    inn.seekg(0, std::ios::beg);
    size = end - start;

    if (size <= 8) {lua_pushnil(L);return 1;}   // Simple check: size is valid?

    buffer = new char[size + 1];                // Read file contents
    inn.read(buffer, size);
    buffer[size] = '\0';

    file.fdata = std::string(buffer);           // Set all struct vars
    delete [] buffer;
    file.pend = file.pstart = 0;
    
    root_name = getXMLtag(&file);               // Get name of the root tag
    root_name = parseXMLname(root_name);
    file.pend = file.pstart = 0;

    root_tag = parseXMLtag(&file, root_name, dummy);    // Start the parsing
    makeLUAtable(root_tag);                             // Algorithm and
    deleteXMLtags(root_tag);                            // make a lua table

    return 1;
}

// ---------- MAKE A LUA TABLE FROM AN XMLTAG ---------------------------------
void makeLUAtable(Tag* t) {
    std::map<std::string, std::string>::iterator a_itr; // Iterator attr map
    std::vector<Tag*>::iterator t_itr;  // Iteratfor for tags vector

    lua_newtable(L);                    // Create the table for THIS TAG
    lua_pushlstring(L, t->name.c_str(), t->name.length());
    lua_setfield(L, -2, "name");        // Set name attribute

    lua_newtable(L);                    // Create table for attributes
    for (a_itr = t->attributes.begin(); a_itr != t->attributes.end(); ++a_itr){
        lua_pushlstring(L, a_itr->second.c_str(), a_itr->second.length());
        lua_setfield(L, -2, a_itr->first.c_str());
    }

    lua_setfield(L, -2, "attr");        // Last table made is ".attr"

    if (t->tag_data.length() == 0) {
        lua_newtable(L);                // Create table for sub tags

        for (int i = 0; i < t->tags.size(); i++) {
            makeLUAtable(t->tags.at(i));// Make table from next tag (recursive)
            lua_rawseti(L, -2, i+1);    // +1 since Lua tables start at 1 not 0
        }

        lua_setfield(L, -2, "tags");    // Last table made is ".t
    }
    else {
        lua_pushlstring(L, t->tag_data.c_str(), t->tag_data.length());
        lua_setfield(L, -2, "data");
    }
}

// ---------- PARSE AN XML TAG ------------------------------------------------
Tag* parseXMLtag(File* f, std::string main_name, bool &sub_ended) {
    Tag* temp_tag, *sub_tag;
    std::string temp_tagstring, temp_name;
    bool tag_ended;

    sub_ended = false;              // Default that this is not a closing tag

    temp_tagstring = getXMLtag(f);
    if (temp_tagstring.empty()) return NULL;

    // Se if it is a terminated / closing tag
    if (temp_tagstring.at(1) == '/') {
        temp_name = "</" + main_name + ">";
        sub_ended = (temp_tagstring == temp_name) ? true : false;
        f->pstart = f->pend;
        return NULL;
    }

    // Create a new TAG and parse: name, attributes, and see if it has txt data
    temp_tag = new Tag;
    temp_tag->name = parseXMLname(temp_tagstring);
    temp_tag->attributes = parseXMLattributes(temp_tagstring);

    // If the second last char is / then this is a closed tag and
    // does not need to be treated anymore
    if (temp_tagstring.at(temp_tagstring.length() - 2) != '/') {
        temp_tag->tag_data = parseXMLtagdata(f);

        // Keep on finding more sub tags and parsing them, and appending
        // each new tag to the vector of tags
        while(sub_tag && !tag_ended) {
            sub_tag = parseXMLtag(f, temp_tag->name, tag_ended);
            //if (!sub_tag && tag_ended) break;
            if (sub_tag) temp_tag->tags.push_back(sub_tag);
        }
    }
    return temp_tag;
}

// ---------- EXTRACT A SINGLE XML TAG <...> ----------------------------------
inline std::string getXMLtag(File* f) {
    // Get the start "<", and end ">" of the next tag to be parsed
    f->pstart = f->fdata.find("<", f->pstart);
    f->pend = f->fdata.find(">", f->pstart+1);
    if (f->pend==std::string::npos || f->pstart==std::string::npos) return "";

    f->pstart += 1;
    return f->fdata.substr((f->pstart-1), ((f->pend - (f->pstart-1)) + 1));
}

// ---------- PARSE AN XML TAG'S NAME -----------------------------------------
inline std::string parseXMLname(std::string s) {
    unsigned int start = s.find_first_not_of("<");
    unsigned int end = s.find_first_of(" ");
    if (end == std::string::npos) end = s.find_last_not_of(">") + 1;
    return s.substr(start, end - start);
}

// ---------- PARSE AN XML TAG'S ATTRIBUTES -----------------------------------
std::map<std::string, std::string> parseXMLattributes(std::string s) {
    std::map<std::string, std::string> temp;
    std::string key, value;
    unsigned int start, sub_start, sub_end;
    
    start = s.find_first_of(" ");
    while (start != std::string::npos) {        
        // Find the KEY
        sub_start = start + 1;
        sub_end = s.find("=", sub_start + 1);
        key = s.substr(sub_start, (sub_end - sub_start));
        
        // Find the VALUE
        sub_start = sub_end + 1;
        sub_end = s.find("\"", sub_start + 1);
        value = s.substr(sub_start + 1, (sub_end - sub_start) - 1);

        // Insert the pair, and find next space before next attribute
        temp.insert(std::pair<std::string, std::string>(key, value));
        start = s.find(" ", sub_end);
    }
    return temp;
}

// ---------- PARSE AN XML TAG'S DATA -----------------------------------------
inline std::string parseXMLtagdata(File* f) {
    unsigned int next_tagstart, n;
    std::string temp;

    next_tagstart = f->fdata.find_first_of("<", f->pend + 1);
    temp = f->fdata.substr(f->pend + 1, (next_tagstart - f->pend)-1);

    for (n = 0; n < temp.length(); n++) {
        if (temp.at(n) != char(32) && temp.at(n) != char(10) &&
            temp.at(n) != char(9)  && temp.at(n) != char(13)) {
            return temp;
        }
    }
    return "";
}

// ---------- DELETE THE TREE OF XML TAGS -------------------------------------
inline void deleteXMLtags(Tag* t) {
    if (!t->tags.empty()) {                     // IF this tag has any sub tags
        while (!t->tags.empty()) {              // Loop untill no tags left
            deleteXMLtags(t->tags.back());      // Delete last tag in vector
            t->tags.pop_back();
        }
    }
    else delete t;
}