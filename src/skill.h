#ifndef SKILL_H_
#define SKILL_H_

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

#include "yaml-cpp/yaml.h"

using Json = nlohmann::json;

struct Skill {
  std::string name;
  std::string description;
  std::vector<std::string> keywords;
  std::string instructions;
};

class SkillManager {
 public:
  SkillManager();

  void Register(Skill skill);
  bool LoadFromDirectory(const std::string& path);
  std::vector<std::string> GetMatchingInstructions(
      const std::string& query) const;
  Json BuildSkillsSchema() const;
  std::vector<Skill> GetAllSkills() const;

 private:
  std::optional<Skill> ParseSkillFile(const std::string& path) const;
  std::vector<Skill> skills_;
};

#endif  // SKILL_H_
