#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace std;
using namespace std::chrono_literals;

const string BILL_FILE = "bill_data.txt";
const string TEMP_BILL = "temp_bill.txt";

struct InventoryItem
{
    string name;
    int rate = 0;
    int quantity = 0;
};

string trim(const string &text)
{
    size_t start = text.find_first_not_of(" \t\r\n");
    size_t end = text.find_last_not_of(" \t\r\n");
    return (start == string::npos) ? string() : text.substr(start, end - start + 1);
}

bool parseInventoryLine(const string &line, InventoryItem &item)
{
    if (line.empty())
    {
        return false;
    }

    size_t first = line.find(':');
    size_t second = (first == string::npos) ? string::npos : line.find(':', first + 1);
    if (first == string::npos || second == string::npos)
    {
        return false;
    }

    string namePart = trim(line.substr(0, first));
    string ratePart = trim(line.substr(first + 1, second - first - 1));
    string quantityPart = trim(line.substr(second + 1));
    if (namePart.empty() || ratePart.empty() || quantityPart.empty())
    {
        return false;
    }

    try
    {
        item.name = namePart;
        item.rate = stoi(ratePart);
        item.quantity = stoi(quantityPart);
    }
    catch (...)  // NOLINT
    {
        return false;
    }

    return true;
}

bool loadInventory(vector<InventoryItem> &inventory)
{
    inventory.clear();
    ifstream in(BILL_FILE);
    if (!in)
    {
        ofstream createFile(BILL_FILE, ios::app);
        return static_cast<bool>(createFile);
    }

    string line;
    while (getline(in, line))
    {
        InventoryItem item;
        if (parseInventoryLine(line, item))
        {
            bool found = false;
            for (auto &entry : inventory)
            {
                if (entry.name == item.name)
                {
                    entry.quantity += item.quantity;
                    entry.rate = item.rate;
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                inventory.push_back(item);
            }
        }
    }
    return true;
}

bool saveInventory(const vector<InventoryItem> &inventory)
{
    ofstream out(TEMP_BILL);
    if (!out)
    {
        return false;
    }

    for (const auto &item : inventory)
    {
        out << item.name << " : " << item.rate << " : " << item.quantity << '\n';
    }
    out.close();
    return rename(TEMP_BILL.c_str(), BILL_FILE.c_str()) == 0;
}

void clearScreen()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void addItem()
{
    while (true)
    {
        clearScreen();
        cout << "\tInventory Management" << endl;
        cout << "\t1. Add/Update item" << endl;
        cout << "\t2. Back to main menu" << endl;
        cout << "\t#Enter choice: ";

        int choice;
        if (!(cin >> choice))
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        if (choice == 2)
        {
            break;
        }
        else if (choice == 1)
        {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            string itemName;
            int rate = 0;
            int quantity = 0;

            cout << "\tEnter item name: ";
            getline(cin, itemName);
            itemName = trim(itemName);
            if (itemName.empty())
            {
                cout << "\tItem name cannot be empty." << endl;
                this_thread::sleep_for(2s);
                continue;
            }

            cout << "\tEnter item rate: ";
            cin >> rate;
            cout << "\tEnter item quantity: ";
            cin >> quantity;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            if (rate <= 0 || quantity <= 0)
            {
                cout << "\tRate and quantity must be positive." << endl;
                this_thread::sleep_for(2s);
                continue;
            }

            vector<InventoryItem> inventory;
            if (!loadInventory(inventory))
            {
                cout << "\tError opening inventory file." << endl;
                this_thread::sleep_for(2s);
                continue;
            }

            bool updated = false;
            for (auto &entry : inventory)
            {
                if (entry.name == itemName)
                {
                    entry.rate = rate;
                    entry.quantity += quantity;
                    updated = true;
                    break;
                }
            }
            if (!updated)
            {
                inventory.push_back({itemName, rate, quantity});
            }

            if (!saveInventory(inventory))
            {
                cout << "\tFailed to save inventory." << endl;
            }
            else
            {
                cout << "\tItem saved successfully." << endl;
            }
            this_thread::sleep_for(2s);
        }
    }
}

void printBill()
{
    vector<InventoryItem> inventory;
    if (!loadInventory(inventory))
    {
        cout << "\tUnable to load inventory." << endl;
        this_thread::sleep_for(2s);
        return;
    }

    int totalAmount = 0;
    bool billOpen = true;
    while (billOpen)
    {
        clearScreen();
        cout << "\tBilling" << endl;
        cout << "\t1. Add item to bill" << endl;
        cout << "\t2. Finish bill" << endl;
        cout << "\t#Enter choice: ";

        int choice;
        if (!(cin >> choice))
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        if (choice == 2)
        {
            billOpen = false;
            continue;
        }
        else if (choice == 1)
        {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            string itemName;
            int quantity = 0;

            cout << "\tEnter item name: ";
            getline(cin, itemName);
            itemName = trim(itemName);
            cout << "\tEnter quantity: ";
            cin >> quantity;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            if (itemName.empty() || quantity <= 0)
            {
                cout << "\tInvalid item name or quantity." << endl;
                this_thread::sleep_for(2s);
                continue;
            }

            bool found = false;
            for (auto &entry : inventory)
            {
                if (entry.name == itemName)
                {
                    found = true;
                    if (quantity <= entry.quantity)
                    {
                        int amount = entry.rate * quantity;
                        entry.quantity -= quantity;
                        totalAmount += amount;
                        cout << "\tItem | Rate | Quantity | Amount" << endl;
                        cout << "\t" << entry.name << " | " << entry.rate << " | " << quantity << " | " << amount << endl;
                    }
                    else
                    {
                        cout << "\tInsufficient stock for '" << entry.name << "'. Available: " << entry.quantity << "" << endl;
                    }
                    break;
                }
            }
            if (!found)
            {
                cout << "\tItem not found in inventory." << endl;
            }
            this_thread::sleep_for(2s);
        }
    }

    if (!saveInventory(inventory))
    {
        cout << "\tWarning: Could not save updated inventory." << endl;
    }

    clearScreen();
    cout << "\n\tTotal Bill Amount: " << totalAmount << "\n";
    cout << "\tThank you for shopping!" << endl;
    this_thread::sleep_for(3s);
}

int main()
{
    bool running = true;
    while (running)
    {
        clearScreen();
        cout << "\tSuper Market Billing System" << endl;
        cout << "\t1. Add or update inventory item" << endl;
        cout << "\t2. Create bill" << endl;
        cout << "\t3. Exit" << endl;
        cout << "\t#Enter choice: ";

        int choice;
        if (!(cin >> choice))
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        if (choice == 1)
        {
            addItem();
        }
        else if (choice == 2)
        {
            printBill();
        }
        else if (choice == 3)
        {
            running = false;
            clearScreen();
            cout << "\tGoodbye!" << endl;
            this_thread::sleep_for(2s);
        }
    }
    return 0;
}
