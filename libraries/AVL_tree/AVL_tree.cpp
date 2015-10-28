#include "AVL_tree.h"

AVL_tree::AVL_tree(byte maxSize){
  mMaxSize = maxSize;
  mSize = 0;
  root = NULL;
  loadEEPROM();
}

AVL_tree::~AVL_tree(){
  Clear();
}

void AVL_tree::Balance(Node& node)
{
  if(BalanceFactor(node) == 2) // Unbalance on right side
    {
      if(BalanceFactor(node->right) < 0) // If left chain longer
	RotateLeft(node->right);
      RotateRight(node);
    }
  if(BalanceFactor(node) == -2)
    {
      if(BalanceFactor(node->left) > 0)
	RotateRight(node->left);
      RotateLeft(node);
    }
}

boolean AVL_tree::Insert(data d, bool saveEEPROM){
  if(mMaxSize < mSize)
    return false;
  if(IsEmpty()){
    root = new TreeNode(d);
    return true;
  }
  else{
    Insert(root, d, saveEEPROM);
    return true;
  }
}

Node AVL_tree::Insert(Node& node, data d, bool save){
  if(node == NULL){ // If empty, insert it...
    node = new TreeNode(d);
    node->status = false;
    node->timerid = 255;
    ++mSize;
    if(save)
      saveEEPROM(node);
    return node;
  }
  /* Now check if we should go left or right */
  else if(d < node->d){ // If left
    Insert(node->left, d, save);
  }
  else if(d > node->d){ // If right
    Insert(node->right, d, save);
  }
  Balance(node);
  return node;
}

boolean AVL_tree::Insert(Node node, bool save){
  if(mMaxSize < mSize)
    return false;

  if(IsEmpty()){
    root = node;
    return true;
  }
  else{
    Insert(root, node, save);
    return true;
  }
}


Node AVL_tree::Insert(Node& node, Node& newNode, bool save){
  if(node == NULL){ // If empty, insert it...
    node = newNode;
    ++mSize;
    if(save)
      saveEEPROM(node);
    return node;
  }
  /* Now check if we should go left or right */
  else if(newNode->d < node->d){ // If left
    Insert(node->left, newNode, save);
  }
  else if(newNode->d > node->d){ // If right
    Insert(node->right, newNode, save);
  }
  Balance(node);
  return node;
}

data AVL_tree::FindMin(){
  return FindMin(root)->d;
}

Node AVL_tree::FindMin(Node node){
  return node->left ? FindMin(node->left) : node;
}

void AVL_tree::RemoveMin(){
  Node min = ExtractMin(root);
  delete(min);
  min = NULL;
  --mSize;
}

Node AVL_tree::ExtractMin(Node& node){
  if(node->left == NULL){
    return node->right;
  }
  node->left = ExtractMin(node->left);
  Balance(node);
  return node;
}

boolean AVL_tree::Remove(data d){
  byte s = mSize;
  root = Remove(root, d);
  if(s == mSize)
    return false;
  else{
    return true;
    saveEEPROM();
  }
}

Node AVL_tree::Remove(Node& node, data d){

  if(node == NULL)
    return NULL;

  if(d < node->d)
    node->left = Remove(node->left,d);
  else if(d > node->d)
    node->right = Remove(node->right,d);
  else{ // d == node->d
    Node l = node->left;
    Node r = node->right;
    delete(node);
    --mSize;
    if(r == NULL)
      return l; 
    Node min = FindMin(r);
    min->right = ExtractMin(r);
    min->left = l;
    Balance(min);
    return min;
  }
  Balance(node);
  return node;
}

void AVL_tree::Clear(){
  Clear(root);
  mSize = 0;
}

void AVL_tree::Clear(Node& node){
  if(node != NULL){
    Clear(node->left);
    Clear(node->right);
    delete(node);
    node = NULL;
  }
}

boolean AVL_tree::IsEmpty() const{
  if(root == NULL)
    return true;
  return false;
}

int AVL_tree::Height(Node node){
  int left = 0; 
  int right = 0;
  
  if(node == NULL){
    return 0;
  }
  left = Height(node->left);
  right = Height(node->right);

  if(left > right){
    return left+1;
  }
  else{
    return right+1;
  }
}

int AVL_tree::BalanceFactor(Node node){
  return Height(node->right) - Height(node->left);
}

void AVL_tree::RotateLeft(Node& node){
  Node otherNode;

  otherNode = node->left;
  node->left = otherNode->right;
  otherNode->right = node;
  node = otherNode;
}

void AVL_tree::RotateRight(Node& node){
  Node otherNode;
  otherNode = node->right;
  node->right = otherNode->left;
  otherNode->left = node;
  node = otherNode;
}

Node& AVL_tree::Find(data d){
  return Find(root, d);
}

Node& AVL_tree::Find(Node& node, data d) {
  if(!node)
    return node;
  /* Now check if we should go left or right */
  else if(d < node->d){ // If left
    return Find(node->left, d);
  }
  else if(d > node->d){ // If right
    return Find(node->right, d);
  }
  return node;
}

boolean AVL_tree::Contains(data id){
  if(Find(id) == NULL)
    return false;
  else
    return true;
}


void AVL_tree::ForEach(ExternalFunction externalFunc){
  ForEach(root, externalFunc);
}

void AVL_tree::ForEach(Node& node, ExternalFunction externalFunc){
  if(node != NULL)
    {
      ForEach(node->left, externalFunc);
      ForEach(node->right, externalFunc);
      externalFunc(node);
    }
}

void AVL_tree::SendNodes(EthernetClient* client){
  //buffer->reserve((sizeof(unsigned char)*15*mSize)+1);
  if(IsEmpty()){
    client->println("-1");
  }
  SendNodes(root, client);
}

void AVL_tree::SendNodes(Node node, EthernetClient* client){
  if(node){
    SendNodes(node->left, client);
    // DO STRING
    String* buffer = new String("");
    buffer->reserve(sizeof(unsigned char)*20);
    *buffer += node->d;
    *buffer += ':';
    if(node->status)
      *buffer += '1';
    else
      *buffer += '0';
    *buffer += ':';
    *buffer += node->timerid;
    *buffer += ':' ;
    *buffer +=  node->onHour;
    *buffer += ':' ;
    *buffer += node->onMinute;
    *buffer += ':' ;
    *buffer += node->offHour;
    *buffer += ':';
    *buffer +=  node->offMinute;
    *buffer += 'N';
    client->print(*buffer);
    Serial.println(*buffer);
    delete buffer;
    SendNodes(node->right, client);
  }
}

/*
 * Save switch_cache in cache into EEPROM
 * This is done when any changes have been done to cache.
 *    Bit: |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |
 * Byte 1: | SId | SId | SId | SId | SId | SId | SId | SId | 
 * Byte 2: | TId | TId | TId | TId | TId | TId | TId | TId | 
 * Byte 3: | OnM1| OnM0| OnH4| OnH3| OnH2| OnH1| OnH0|Status| 
 * Byte 4: |OffH3|OffH2|OffH1|OffH0| OnM5| OnM4| OnM3| OnM2|
 * Byte 5: |NONE |OffM5|OffM4|OffM3|OffM2|OffM1|OffM0|OffH4|
 * 
 * OnHour, OffHour = 5 bit
 * OnMinute, OffMinute = 6 bit
 * Status = 1 bit
 */
void AVL_tree::saveEEPROM()
{
  unsigned int addr = 1;
  saveAllEEPROM(root, addr);
  EEPROM.write(0, mSize);
}

void AVL_tree::saveAllEEPROM(Node node, unsigned int& addr){
  if (node == NULL)
    return;

  saveEEPROM(node, addr);

  saveAllEEPROM(node->left, addr);
  saveAllEEPROM(node->right, addr);
}


// NEW
void AVL_tree::saveEEPROM(Node node){
  byte count = EEPROM.read(0); // How many switches in memory
  byte bytePerNode = 5;
  bool found = false;
  for(unsigned int i = 1; i <= count; ++i){
    unsigned int startAddr = (i * bytePerNode) - (bytePerNode - 1);
    if(EEPROM.read(startAddr) == node->d){
      saveEEPROM(node, startAddr);
      found = true;
      break;
    }
  }
  if(!found){
    unsigned int addr = (count * bytePerNode) + 1;
    saveEEPROM(node, addr);
    EEPROM.write(0, mSize);
  }
}

void AVL_tree::saveEEPROM(Node node, unsigned int& addr)
{
  Serial.print(F("Saving... ID: "));
  Serial.print( node->d );
  Serial.print(F(", First Addr: "));
  Serial.print( addr );
  // Byte 1: Write controller
  EEPROM.write((addr)++, node->d);
  // Byte 2: | TId | TId | TId | TId | TId | TId | TId | TId | 
  EEPROM.write((addr)++, node->timerid);
  // Byte 3: | OnM1| OnM0| OnH4| OnH3| OnH2| OnH1| OnH0|Status| 
  byte tmp = B00000011 & node->onMinute; // Get 2 least significant bits;
  tmp = tmp << 5;                  // Get space for onHour
  tmp = tmp | node->onHour;        // Insert onHour
  tmp = tmp << 1;                  // Get space for status
  if(node->status)                 // Insert status
    tmp = tmp | B00000001;
  else
    tmp = tmp | B00000000;
  EEPROM.write((addr)++, tmp); 
  // Byte 4: |OffH3|OffH2|OffH1|OffH0| OnM5| OnM4| OnM3| OnM2| 0-3 offHour, 2-5 onMinute
  tmp = B00001111 & node->offHour; // Insert 4 first bits
  tmp = tmp << 4;
  byte rest = B00111100 & node->onMinute;// Parse what's left of onMinute
  rest = rest >> 2;
  tmp = tmp | rest;                // Insert rest at front
  EEPROM.write((addr)++, tmp);
  //Byte 5: |NONE |OffM5|OffM4|OffM3|OffM2|OffM1|OffM0|OffH4|
  tmp = B00111111 & node->offMinute;
  tmp = tmp << 1;
  rest = B00010000 & node->offHour;
  rest = rest >> 4;
  tmp = tmp | rest;
  EEPROM.write(addr, tmp);
  Serial.print(F(", Last Addr: "));
  Serial.print( addr );
  Serial.println(F(" ... DONE!"));
}

/*
 * Load switch_cache in EEPROM into cache.
 * This should only be done at setup state.
 */
void AVL_tree::loadEEPROM()
{
  byte count = EEPROM.read(0); // How many switch_cache in memory
  byte loaded_count = 0; // count_pos
  unsigned int addr = 1; // Current EEPROM address
  while(loaded_count < count)
  {
    Node newNode = new TreeNode();
     // Read controller
     // Byte 1: | SId | SId | SId | SId | SId | SId | SId | SId | 
     newNode->d = EEPROM.read(addr++);
     // Byte 2: | TId | TId | TId | TId | TId | TId | TId | TId | 
     newNode->timerid = EEPROM.read(addr++);
     // Byte 3: | OnM1| OnM0| OnH4| OnH3| OnH2| OnH1| OnH0|Status| 
     byte in = EEPROM.read(addr++);
     byte tmp = in & B00000001;
     if(tmp == 1){
       newNode->status = true;
     }else{
       newNode->status = false;
     }
     tmp = in & B00111110;
     tmp = tmp >> 1;
     newNode->onHour = tmp;
     tmp = in & B11000000;
     tmp = (unsigned int)tmp >> 6;
     
     // Byte 4: |OffH3|OffH2|OffH1|OffH0| OnM5| OnM4| OnM3| OnM2|
     in = EEPROM.read(addr++);
     byte rest = in & B00001111;
     rest = rest << 2;
     tmp = tmp | rest;
     newNode->onMinute = tmp;
     tmp = in & B11110000;
     tmp = (unsigned int)tmp >> 4;
     
      // Byte 5: |NONE |OffM5|OffM4|OffM3|OffM2|OffM1|OffM0|OffH4|
     in = EEPROM.read(addr++);
     rest = in & B00000001;
     rest = rest << 4;
     tmp = tmp | rest;
     newNode->offHour = tmp;
     tmp = in & B01111110;
     tmp = tmp >> 1;
     newNode->offMinute = tmp;
     Insert(root, newNode, false);
     Serial.print(F("Loaded..."));
     Serial.print(loaded_count);
     Serial.print(F(" addr..."));
     Serial.println(addr);
     ++loaded_count;
  }
  Serial.println(F("Load switches from memory... DONE!"));
}

void AVL_tree::SetStatus(byte id, byte status)
{
  TreeNode* node = Find(id);
  if(node == NULL){
    Serial.println(F("Node NOT Found!"));
    return;
  }
  Serial.println(F("Node Found! ID: "));
  Serial.println(node->d);
  if (status == 1){
    node->status = true;
    Serial.println(F("Status true!"));
  }
  else{
    node->status = false;
    Serial.println(F("Status false!"));
  }
}

// Set Timer => timerid:onHour:onMinute:offHour:offMinute:switchidN:switchidK:....:switchidZ:
void AVL_tree::SetTimer(byte*& id_arr, byte timerid, byte onHour, byte onMinute, byte offHour, byte offMinute)
{
  while(*id_arr != 0 || id_arr == NULL)
    {
      Node node = Find(*id_arr);
      if( node != NULL)
	{
	  Serial.print(F("Setting timer on node: "));
	  Serial.println(node->d);
	  node->timerid = timerid;
	  node->onHour = onHour;
	  node->onMinute = onMinute;
	  node->offHour = offHour;
	  node->offMinute = offMinute;
	}
      id_arr++;
    }
  saveEEPROM();
}

void AVL_tree::RemoveTimer(const byte& timerid){
  RemoveTimer(root, timerid);
  saveEEPROM();
}

void AVL_tree::RemoveTimer(Node node, const byte& timerid){
  if(node == NULL){
    return;
  }
  if(node->timerid == timerid){
    node->timerid = 255;
  }
  RemoveTimer(node->left, timerid);
  RemoveTimer(node->right, timerid);
}
