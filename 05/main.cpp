
#include <iostream>
#include <pqxx/pqxx>

// Структура, представляющая клиента
struct Client {
    std::string first_name;
    std::string last_name;
    std::string email;
    std::vector<std::string> phone_numbers;
};

class ClientManager {
public:
    // Конструктор класса ClientManager
    ClientManager() {
        // Устанавливаем соединение с базой данных
        connection = std::make_unique<pqxx::connection>(
            "host=localhost "
            "port=5432 "
            "dbname=DBNAME "
            "user=USER "
            "password=PASS");
    }

    // Метод, создающий структуру БД (таблицы)
    void createDatabaseStructure() {
        std::string createClientsTableQuery = "CREATE TABLE IF NOT EXISTS clients ("
            "id SERIAL PRIMARY KEY,"
            "first_name TEXT,"
            "last_name TEXT,"
            "email TEXT"
            ");";

        std::string createPhonesTableQuery = "CREATE TABLE IF NOT EXISTS phones ("
            "client_id INT REFERENCES clients(id),"
            "phone_number TEXT"
            ");";

        pqxx::work transaction(*connection);
        transaction.exec(createClientsTableQuery);
        transaction.exec(createPhonesTableQuery);
        transaction.commit();
    }

    // Метод, позволяющий добавить нового клиента
    void addClient(const Client& client) {
        pqxx::work transaction(*connection);

        std::string insertClientQuery = "INSERT INTO clients (first_name, last_name, email) "
            "VALUES (" + transaction.quote(client.first_name) + ", " +
            transaction.quote(client.last_name) + ", " +
            transaction.quote(client.email) + ") RETURNING id;";

        pqxx::result result = transaction.exec(insertClientQuery);
        if (!result.empty()) {
            int clientId = result[0][0].as<int>();
            addPhones(clientId, client.phone_numbers, transaction);
        }

        transaction.commit();
    }

    // Метод, позволяющий добавить телефон для существующего клиента
    void addPhoneForClient(int clientId, const std::string& phoneNumber) {
        pqxx::work transaction(*connection);

        std::string insertPhoneQuery = "INSERT INTO phones (client_id, phone_number) "
            "VALUES (" + std::to_string(clientId) + ", " +
            transaction.quote(phoneNumber) + ");";

        transaction.exec(insertPhoneQuery);
        transaction.commit();
    }

    // Метод, позволяющий изменить данные о клиенте
    void updateClient(int clientId, const Client& client) {
        pqxx::work transaction(*connection);

        std::string updateClientQuery = "UPDATE clients "
            "SET first_name = " + transaction.quote(client.first_name) + ", "
            "last_name = " + transaction.quote(client.last_name) + ", "
            "email = " + transaction.quote(client.email) + " "
            "WHERE id = " + std::to_string(clientId) + ";";

        transaction.exec(updateClientQuery);
        transaction.commit();
    }

    // Метод, позволяющий удалить телефон для существующего клиента
    void deletePhoneForClient(int clientId, const std::string& phoneNumber) {
        pqxx::work transaction(*connection);

        std::string deletePhoneQuery = "DELETE FROM phones "
            "WHERE client_id = " + std::to_string(clientId) + " "
            "AND phone_number = " + transaction.quote(phoneNumber) + ";";

        transaction.exec(deletePhoneQuery);
        transaction.commit();
    }

    // Метод, позволяющий удалить существующего клиента
    void deleteClient(int clientId) {
        pqxx::work transaction(*connection);

        std::string deletePhonesQuery = "DELETE FROM phones "
            "WHERE client_id = " + std::to_string(clientId) + ";";

        std::string deleteClientQuery = "DELETE FROM clients "
            "WHERE id = " + std::to_string(clientId) + ";";

        transaction.exec(deletePhonesQuery);
        transaction.exec(deleteClientQuery);
        transaction.commit();
    }

    // Метод, позволяющий найти клиента по его данным (имени, фамилии, email-у или телефону)
    std::vector<Client> findClients(const std::string& searchCriteria) {
        std::string searchClientsQuery = "SELECT * "
            "FROM clients "
            "LEFT JOIN phones "
            "ON clients.id = phones.client_id "
            "WHERE first_name ILIKE '%" + searchCriteria + "%' OR "
            "last_name ILIKE '%" + searchCriteria + "%' OR "
            "email ILIKE '%" + searchCriteria + "%' OR "
            "phone_number ILIKE '%" + searchCriteria + "%';";

        pqxx::nontransaction transaction(*connection);
        pqxx::result result = transaction.exec(searchClientsQuery);

        std::vector<Client> clients;
        std::unordered_map<int, Client> clientMap;

        for (const auto& row : result) {
            int clientId = row["id"].as<int>();
            auto clientIt = clientMap.find(clientId);

            if (clientIt == clientMap.end()) {
                Client client;
                client.first_name = row["first_name"].as<std::string>();
                client.last_name = row["last_name"].as<std::string>();
                client.email = row["email"].as<std::string>();

                if (!row["phone_number"].is_null()) {
                    client.phone_numbers.push_back(row["phone_number"].as<std::string>());
                }

                clientMap.emplace(clientId, client);
            }
            else {
                if (!row["phone_number"].is_null()) {
                    clientIt->second.phone_numbers.push_back(row["phone_number"].as<std::string>());
                }
            }
        }

        for (const auto& [_, client] : clientMap) {
            clients.push_back(client);
        }

        return clients;
    }

private:
    std::unique_ptr<pqxx::connection> connection;

    // Вспомогательный метод для добавления телефонов клиента
    void addPhones(int clientId, const std::vector<std::string>& phoneNumbers, pqxx::work& transaction) {
        for (const auto& phoneNumber : phoneNumbers) {
            std::string insertPhoneQuery = "INSERT INTO phones (client_id, phone_number) "
                "VALUES (" + std::to_string(clientId) + ", " +
                transaction.quote(phoneNumber) + ");";

            transaction.exec(insertPhoneQuery);
        }
    }
};

int main() {
    ClientManager clientManager;

    // Создаем структуру БД
    clientManager.createDatabaseStructure();

    // Добавляем клиентов
    Client client1;
    client1.first_name = "John";
    client1.last_name = "Doe";
    client1.email = "john.doe@example.com";
    client1.phone_numbers = { "1234567890", "9876543210" };
    clientManager.addClient(client1);

    Client client2;
    client2.first_name = "Jane";
    client2.last_name = "Smith";
    client2.email = "jane.smith@example.com";
    client2.phone_numbers = { "1112223333" };
    clientManager.addClient(client2);

    // Добавляем телефон для существующего клиента
    clientManager.addPhoneForClient(1, "5555555555");

    // Изменяем данные о клиенте
    Client updatedClient;
    updatedClient.first_name = "John";
    updatedClient.last_name = "Doe";
    updatedClient.email = "john.doe@example.com";
    clientManager.updateClient(1, updatedClient);

    // Удаляем телефон для существующего клиента
    clientManager.deletePhoneForClient(1, "5555555555");

    // Удаляем существующего клиента
    clientManager.deleteClient(1);

    // Находим клиента по данным
    std::vector<Client> foundClients = clientManager.findClients("John");
    for (const auto& client : foundClients) {
        std::cout << "First Name: " << client.first_name << std::endl;
        std::cout << "Last Name: " << client.last_name << std::endl;
        std::cout << "Email: " << client.email << std::endl;
        std::cout << "Phone Numbers: ";
        for (const auto& phoneNumber : client.phone_numbers) {
            std::cout << phoneNumber << ", ";
        }
        std::cout << std::endl;
    }

    return 0;
}