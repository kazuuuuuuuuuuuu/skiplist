// 模板类 类声明和实现都要写在.h中
#ifndef _SKIPLIST_H_
#define _SKIPLIST_H_

#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <cstdlib> // 包含 rand() 和 srand() 在构造函数时输入种子
#include <ctime>   // 包含 time()
#include <string>
#include <mutex>

#define STORE_FILE "dumpFile.txt"
const std::string delimiter = ":";

template<typename K, typename V>
class Node // 用于存储键值对
{
private:
	K key;
	V value;

public:
	int node_level; // 节点在跳表中的层级位置
	Node<K, V>** forward; // 指针的指针 -> 指针数组 -> 记录该节点在不同层级的下一个节点的地址

public:
	Node(K k, V v, int level);
	~Node();
	K get_key() const;
	V get_value() const;
	void set_value(V v);
};

template<typename K, typename V>
Node<K, V>::Node(K k, V v, int level)
{
    key = k;
    value = v;
    node_level = level;
    forward = new Node<K, V>* [level + 1];
    for (int i = 0; i <= level; i++)
    {
        forward[i] = nullptr;
    }
}

template<typename K, typename V>
Node<K, V>::~Node()
{
    delete[] forward;
}

template<typename K, typename V>
K Node<K, V>::get_key() const
{
    return key;
}

template<typename K, typename V>
V Node<K, V>::get_value() const
{
    return value;
}

template<typename K, typename V>
void Node<K, V>::set_value(V v)
{
    value = v;
}

template<typename K, typename V>
class SkipList
{
private:
	int max_level_; // 跳表中允许的最大层数
	int skip_list_level_; // 跳表当前的层数
    int element_count_; // 所有结点的数量
	Node<K, V>* header_; // 跳表的头结点
	
    std::ofstream file_writer_; // 文件写入器
	std::ifstream file_reader_; // 文件读取器
    
    std::mutex mtx;

private:
    bool is_valid_string(const std::string& str);
    void get_key_value_from_string(const std::string& str, std::string* key, std::string* value);

public:
	SkipList(int max_level);
	~SkipList();
	
    int get_random_level(); // 获取随机层数
    int size();  // 跳表中的节点个数
	Node<K, V>* create_node(K k, V v, int level); // 节点创建
	
    bool search_element(K key);  // 搜索节点
	int insert_element(K key, V value); // 插入节点
	void delete_element(K key); // 删除节点
	
	void dump_file(); // 持久化数据到文件
	void load_file(); // 从文件加载数据
    
    void display_list();  // 展示所有节点
    void clear(Node<K, V>* node);  // 递归删除节点
};

template<typename K, typename V>
SkipList<K, V>::SkipList(int max_level)
{
    max_level_ = max_level;
    skip_list_level_ = 0;
    element_count_ = 0;
    K k; // 默认值 无影响 从头结点的下个节点开始比较
    V v; // 默认值
    // 头结点的指针数组指向 0~max_level 层的第一个实际节点
    header_ = new Node<K, V>(k, v, max_level);
}

template<typename K, typename V>
SkipList<K, V>::~SkipList()
{
    if (file_writer_.is_open())
    {
        file_writer_.close();
    }

    if (file_reader_.is_open())
    {
        file_reader_.close();
    }

    if (element_count_ != 0)
    {
        clear(header_->forward[0]);
    }

    delete header_;
}

template<typename K, typename V>
int SkipList<K, V>::get_random_level()
{
    // 从第0层开始，所有节点都会出现在这一层
    int level = 0;
    // 0.5的概率为1， 0.5的概率为0
    while (rand() % 2 == 0)
    {
        level++;
    }
    // 控制最高的层数
    level = (level < max_level_) ? level : max_level_;
    return level;
}

// 创建节点，以便于进一步在跳表中进行插入操作。
template<typename K, typename V>
Node<K, V>* SkipList<K, V>::create_node(K k, V v, int level)
{
    Node<K, V>* temp = new Node<K, V>(k, v, level);
    return temp;
}

template <typename K, typename V>
bool SkipList<K, V>::search_element(K key)
{
    Node<K, V>* curr = header_;
    // 从跳表的最高层开始搜索
    for (int i = skip_list_level_; i >= 0; i--)
    {
        // 下一个节点的key小于需要查找的key -> 到下一个节点
        while (curr->forward[i] && curr->forward[i]->get_key() < key)
        {
            curr = curr->forward[i];
        }
        // 当前节点<key && 下一个节点不存在或者>=key -> 下沉
    }
    // 第0层下一个节点 不存在或者>=key 
    curr = curr->forward[0];
    if (curr && curr->get_key() == key)
    {
        mtx.lock();
        std::cout << "search_element succeed: " << key << std::endl; 
        mtx.unlock();
        return true;
    }
    mtx.lock();
    std::cout << "search_element failed: " << key << std::endl; 
    mtx.unlock();
    return false;
}

template <typename K, typename V>
int SkipList<K, V>::insert_element(K key, V value)
{
    mtx.lock();

    Node<K, V>* curr = header_;
    Node<K, V>* update[max_level_ + 1]; // 用于记录每层中需要修改的节点 -> 只修改当层的指针
    for(int i=0;i<=max_level_;i++)
    {
        update[i] = nullptr;
    }

    for (int i = skip_list_level_; i >= 0; i--)
    {
        while (curr->forward[i] && curr->forward[i]->get_key() < key)
        {
            curr = curr->forward[i];
        }
        // 当前节点<key && 下一个节点不存在或者>=key
        // 记录需要修改的节点
        update[i] = curr;
    }

    curr = curr->forward[0];
    // 已存在
    if (curr && curr->get_key() == key)
    {
        mtx.unlock();
        return 1;
    }

    int random_level = get_random_level();
    Node<K, V>* inserted_node = create_node(key, value, random_level);
    // 新节点层数高于当前所有层数 -> 修改当前层高
    if (random_level > skip_list_level_)
    {
        for (int i = skip_list_level_ + 1; i <= random_level; i++)
        {
            update[i] = header_;
        }
        skip_list_level_ = random_level;
    }

    // 每层看作一个链表 -> 更新
    for (int i = 0; i <= random_level; i++)
    {
        inserted_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = inserted_node;
    }
    element_count_++;
    std::cout << "insert_element: " << key << " : " << value << std::endl; 
    mtx.unlock();
    return 0;
}

template<typename K, typename V>
void SkipList<K, V>::delete_element(K key)
{
    mtx.lock();
    Node<K, V>* curr = header_;
    Node<K, V>* update[max_level_ + 1];
    for(int i=0;i<=max_level_;i++)
    {
        update[i] = nullptr;
    }


    for (int i = skip_list_level_; i >= 0; i--)
    {
        while (curr->forward[i] && curr->forward[i]->get_key() < key)
        {
            curr = curr->forward[i];
        }
        // 记录需要修改的节点
        update[i] = curr;
    }
    curr = curr->forward[0];
    // 节点不存在
    if (curr == nullptr || curr->get_key() != key)
    {
        mtx.unlock();
        return;
    }

    for (int i = 0; i <= skip_list_level_; i++)
    {
        // 从下往上修改每一层链表
        if (update[i]->forward[i] != curr)
        {
            break; // 该层未找到 更高的层都不会找到
        }
        update[i]->forward[i] = curr->forward[i];
    }
    // 修改当前高度
    while (skip_list_level_ > 0 && header_->forward[skip_list_level_] == nullptr)
    {
        skip_list_level_--;
    }
    // 释放内存
    delete curr;
    element_count_--;
    std::cout << "delete_element: " << key << std::endl; 
    mtx.unlock();
    return;
}

template<typename K, typename V>
void SkipList<K, V>::display_list()
{
    for (int i = skip_list_level_; i >= 0; i--)
    {
        Node<K, V>* curr = header_->forward[i];
        std::cout << "Level " << i << ": ";
        while (curr)
        {
            std::cout << curr->get_key() << ":" << curr->get_value() << ";";
            curr = curr->forward[i];
        }
        std::cout << std::endl;
    }
}

// 写入文件 只考虑键值对(第0层包含所有) 丢弃索引
template<typename K, typename V>
void SkipList<K, V>::dump_file()
{
    file_writer_.open(STORE_FILE);
    Node<K, V>* curr = header_->forward[0];

    while (curr)
    {
        file_writer_ << curr->get_key() << ":" << curr->get_value() << ";\n";
        curr = curr->forward[0];
    }

    file_writer_.flush();
    file_writer_.close();
}

template<typename K, typename V>
bool SkipList<K, V>::is_valid_string(const std::string& str)
{
    return !str.empty() && str.find(delimiter) != std::string::npos;
}

template<typename K, typename V>
void SkipList<K, V>::get_key_value_from_string(const std::string& str, std::string* key, std::string* value)
{
    if (!is_valid_string(str))
    {
        return;
    }
    *key = str.substr(0, str.find(delimiter)); // str.find(delimiter) 返回的是下标
    *value = str.substr(str.find(delimiter) + 1, str.length());
}

template<typename K, typename V>
void SkipList<K, V>::load_file()
{
    file_reader_.open(STORE_FILE);
    std::string line;
    std::string* key = new std::string;
    std::string* value = new std::string;

    while (getline(file_reader_, line))
    {
        get_key_value_from_string(line, key, value);
        if (key->empty() || value->empty())
        {
            continue;
        }
        insert_element(stoi(*key), *value);
    }

    delete key;
    delete value;
    file_reader_.close();
}

// 递归删除 除了头结点外的所有元素 -> 删第0层即可 所有元素都在第0层
template<typename K, typename V>
void SkipList<K, V>::clear(Node<K, V>* node)
{
    if (node)
    {
        clear(node->forward[0]);
        delete node;
    }
}

template<typename K, typename V>
int SkipList<K, V>::size()
{
    return element_count_;
}

#endif // !_SKIPLIST_H_

