#include <string>
#include <Wt/Dbo/Dbo>
#include <Wt/Dbo/WtSqlTraits>

class Publisher {
public:
    int id;
    std::string name;

    template<class Action>
    void persist(Action& a) {
        Wt::Dbo::field(a, id, "id");
        Wt::Dbo::field(a, name, "name");
    }
};