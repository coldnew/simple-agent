#include "skill.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace {

std::string ToLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return s;
}

// Characters that are sentence/list punctuation, not part of programming
// language names (e.g. c++, c#, .net).
bool IsSentencePunctuation(char c) {
  return c == '.' || c == ',' || c == ':' || c == ';' || c == '!' || c == '?' ||
         c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' ||
         c == '"' || c == '\'';
}

// Strip leading/trailing sentence punctuation from a word, preserving
// characters like +, #, / that appear in language names.
std::string StripPunctuation(const std::string& word) {
  size_t start = 0;
  while (start < word.size() && IsSentencePunctuation(word[start])) {
    ++start;
  }
  size_t end = word.size();
  while (end > start && IsSentencePunctuation(word[end - 1])) {
    --end;
  }
  return word.substr(start, end - start);
}

// Tokenize the user query into lowercase words for matching.
std::vector<std::string> TokenizeQuery(const std::string& query) {
  std::vector<std::string> tokens;
  std::istringstream iss(query);
  std::string word;
  while (iss >> word) {
    std::string cleaned = ToLower(StripPunctuation(word));
    if (!cleaned.empty()) {
      tokens.push_back(cleaned);
    }
  }
  return tokens;
}

}  // namespace

SkillManager::SkillManager() {}

void SkillManager::Register(Skill skill) {
  skills_.push_back(std::move(skill));
}

bool SkillManager::LoadFromDirectory(const std::string& path) {
  std::error_code ec;
  if (!std::filesystem::is_directory(path, ec)) {
    return false;
  }

  for (const auto& entry : std::filesystem::directory_iterator(path)) {
    if (entry.is_directory(ec)) {
      auto skill_path = entry.path() / "SKILL.md";
      std::error_code ec2;
      auto st = std::filesystem::status(skill_path, ec2);
      if (std::filesystem::is_regular_file(st)) {
        auto skill = ParseSkillFile(skill_path.string());
        if (skill) {
          skills_.push_back(std::move(*skill));
        }
      }
    }
  }
  return !skills_.empty();
}

// Split a markdown file with YAML frontmatter into (yaml, body) parts.
// Returns false if the file doesn't have valid frontmatter delimiters.
bool SplitFrontmatter(std::istream& input,
                      std::string* yaml_out,
                      std::string* body_out) {
  std::string line;
  bool in_frontmatter = false;

  while (std::getline(input, line)) {
    if (!in_frontmatter && line.find("---") == 0) {
      in_frontmatter = true;
      continue;
    }
    if (in_frontmatter && line.find("---") == 0) {
      std::stringstream rest;
      rest << input.rdbuf();
      *body_out = rest.str();
      return true;
    }
    if (in_frontmatter) {
      *yaml_out += line + "\n";
    }
  }
  return false;
}

// Parse YAML frontmatter fields into a Skill struct.
std::optional<Skill> ParseFrontmatter(const std::string& yaml_content) {
  try {
    YAML::Node yaml = YAML::Load(yaml_content);

    Skill skill;
    if (yaml["name"]) {
      skill.name = yaml["name"].as<std::string>();
    }
    if (yaml["description"]) {
      skill.description = yaml["description"].as<std::string>();
    }
    if (yaml["keywords"] && yaml["keywords"].IsSequence()) {
      for (const auto& kw : yaml["keywords"]) {
        skill.keywords.push_back(ToLower(kw.as<std::string>()));
      }
    }

    if (skill.name.empty()) {
      return std::nullopt;
    }
    return skill;
  } catch (...) {
    return std::nullopt;
  }
}

std::optional<Skill> SkillManager::ParseSkillFile(
    const std::string& path) const {
  std::ifstream file(path);
  if (!file.is_open()) {
    return std::nullopt;
  }

  std::string yaml_content;
  std::string body;
  if (!SplitFrontmatter(file, &yaml_content, &body)) {
    return std::nullopt;
  }

  auto skill = ParseFrontmatter(yaml_content);
  if (!skill) {
    return std::nullopt;
  }

  // Trim leading/trailing whitespace from the markdown body.
  while (!body.empty() && (body.front() == '\n' || body.front() == '\r')) {
    body.erase(body.begin());
  }
  while (!body.empty() && (body.back() == '\n' || body.back() == '\r')) {
    body.pop_back();
  }
  skill->instructions = body;

  return skill;
}

std::vector<std::string> SkillManager::GetMatchingInstructions(
    const std::string& query) const {
  const auto query_tokens = TokenizeQuery(query);
  std::vector<std::string> instructions;
  for (const auto& skill : skills_) {
    for (const auto& keyword : skill.keywords) {
      for (const auto& token : query_tokens) {
        if (token == keyword) {
          instructions.push_back(skill.instructions);
          goto next_skill;
        }
      }
    }
  next_skill:;
  }
  return instructions;
}

Json SkillManager::BuildSkillsSchema() const {
  Json schema = Json::array();
  for (const auto& skill : skills_) {
    schema.push_back(
        {{"name", skill.name}, {"description", skill.description}});
  }
  return schema;
}

std::vector<Skill> SkillManager::GetAllSkills() const {
  return skills_;
}
