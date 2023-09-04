#include <iostream>
#include <string>
#include <Wt/Dbo/Dbo>
#include <pqxx/pqxx>

// Определение классов Publisher, Book, Stock, Sale, Shop
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

class Book {
public:
    int id;
    std::string title;
    Wt::Dbo::ptr<Publisher> publisher;

    template<class Action>
    void persist(Action& a) {
        Wt::Dbo::field(a, id, "id");
        Wt::Dbo::field(a, title, "title");
        Wt::Dbo::belongsTo(a, publisher, "id_publisher");
    }
};

class Stock {
public:
    int id;
    Wt::Dbo::ptr<Book> book;
    Wt::Dbo::ptr<Shop> shop;
    int count;

    template<class Action>
    void persist(Action& a) {
        Wt::Dbo::field(a, id, "id");
        Wt::Dbo::belongsTo(a, book, "id_book");
        Wt::Dbo::belongsTo(a, shop, "id_shop");
        Wt::Dbo::field(a, count, "count");
    }
};

class Sale {
public:
    int id;
    double price;
    Wt::Dbo::DateTime date_sale;
    Wt::Dbo::ptr<Stock> stock;
    int count;

    template<class Action>
    void persist(Action& a) {
        Wt::Dbo::field(a, id, "id");
        Wt::Dbo::field(a, price, "price");
        Wt::Dbo::field(a, date_sale, "date_sale");
        Wt::Dbo::belongsTo(a, stock, "id_stock");
        Wt::Dbo::field(a, count, "count");
    }
};

class Shop {
public:
    int id;
    std::string name;

    template<class Action>
    void persist(Action& a) {
        Wt::Dbo::field(a, id, "id");
        Wt::Dbo::field(a, name, "name");
    }
};

int main() {
    try {
        // Подключение к базе данных PostgreSQL
        pqxx::connection conn("dbname=mydb user=myuser password=mypassword host=localhost port=5432");
        if (!conn.is_open()) {
            std::cerr << "Failed to connect to PostgreSQL" << std::endl;
            return 1;
        }

        // Инициализация Wt::Dbo с использованием PostgreSQL
        Wt::Dbo::backend::Postgres pg(conn);
        Wt::Dbo::Session session;
        session.setConnection(pg);

        // Создание таблиц (если они не существуют)
        session.createTables();

        // Заполнение таблиц тестовыми данными (пример)
        Wt::Dbo::Transaction transaction(session);

        Wt::Dbo::ptr<Publisher> publisher1 = session.add(new Publisher{ "1", "Publisher1" });
        Wt::Dbo::ptr<Publisher> publisher2 = session.add(new Publisher{ "2", "Publisher2" });

        Wt::Dbo::ptr<Shop> shop1 = session.add(new Shop{ "1", "Shop1" });
        Wt::Dbo::ptr<Shop> shop2 = session.add(new Shop{ "2", "Shop2" });

        Wt::Dbo::ptr<Book> book1 = session.add(new Book{ "1", "Book1", publisher1 });
        Wt::Dbo::ptr<Book> book2 = session.add(new Book{ "2", "Book2", publisher2 });

        Wt::Dbo::ptr<Stock> stock1 = session.add(new Stock{ "1", book1, shop1, 10 });
        Wt::Dbo::ptr<Stock> stock2 = session.add(new Stock{ "2", book2, shop2, 5 });

        Wt::Dbo::ptr<Sale> sale1 = session.add(new Sale{ "1", 20.0, Wt::Dbo::cpp17::make_unique<Wt::WDateTime>(Wt::WDateTime::currentDateTime()), stock1, 2 });
        Wt::Dbo::ptr<Sale> sale2 = session.add(new Sale{ "2", 15.0, Wt::Dbo::cpp17::make_unique<Wt::WDateTime>(Wt::WDateTime::currentDateTime()), stock2, 3 });

        transaction.commit();

        // Ввод имени или идентификатора издателя
        std::string publisherName;
        std::cout << "Введите имя или идентификатор издателя: ";
        std::cin >> publisherName;

        // Выполнение запроса на выборку магазинов
        Wt::Dbo::Transaction queryTransaction(session);
        Wt::Dbo::Query<Shop> query = session.query<Shop>();
        query.where("id IN (SELECT stock.id_shop FROM stock INNER JOIN book ON stock.id_book = book.id INNER JOIN publisher ON book.id_publisher = publisher.id WHERE publisher.name = ?)").bind(publisherName);

        std::vector<Wt::Dbo::ptr<Shop>> shops = query.resultList();
        queryTransaction.commit();

        // Вывод результатов
        std::cout << "Магазины, в которых продают книги издателя '" << publisherName << "':" << std::endl;
        for (const auto& shop : shops) {
            std::cout << "Магазин ID: " << shop.id << ", Название: " << shop.name << std::endl;
        }

        conn.disconnect();
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
