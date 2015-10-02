#ifndef __AVL_TREE__
#define __AVL_TREE__

#include <Arduino.h>
#include <EEPROM.h>
#include <Ethernet.h>

typedef byte data;
//typedef struct TreeNode* Node;

typedef struct TreeNode{
  data d;
  boolean status;
  byte timerid;
  byte offHour;
  byte offMinute;
  byte onHour;
  byte onMinute;
  struct TreeNode *left;
  struct TreeNode *right;
  TreeNode(data k) { d = k; left = right = NULL; }
  TreeNode() { left = right = NULL; }
} *Node;

typedef void(*ExternalFunction)(Node&);

class AVL_tree{
 public:

  AVL_tree(byte maxSize);
  ~AVL_tree();

  boolean Insert(Node node);
  boolean Insert(data d);
  Node& Find(data d);
  boolean Contains(data id);
  boolean Remove(data d);
  void Clear();
  data FindMin();
  void RemoveMin();
  void ForEach(ExternalFunction externalFunc);
  boolean IsEmpty() const;
  // TODO: Add saveEEPROM(Node node) ( and use it where it could be used )
  void saveEEPROM();
  void loadEEPROM();
  void SendNodes(EthernetClient* client);
  void SetStatus(byte id, byte status);
  byte Size(){return mSize;}
  void SetTimer(byte*& id_arr, byte timerid, byte onHour, byte onMinute, byte offHour, byte offMinute);
  void RemoveTimer(const byte& timerid);

 private:
  void saveEEPROM(Node node, unsigned int*& addr);
  Node Insert(Node& node, Node& newNode);
  Node Insert(Node& node, data d);
  int Height(Node node);
  void RotateLeft(Node& node);
  void RotateRight(Node& node);
  void Balance(Node& node);
  int BalanceFactor(Node node);
  void Clear(Node& node);
  Node FindMin(Node node);
  Node ExtractMin(Node& node);
  Node Remove(Node& node, data d);
  Node& Find(Node& node, data d);
  void ForEach(Node& node, ExternalFunction externalFunc);
  void SendNodes(Node node, EthernetClient* client);
  void RemoveTimer(Node node, const byte& timerid);
  Node root; 
  byte mMaxSize;
  byte mSize;
};


#endif
