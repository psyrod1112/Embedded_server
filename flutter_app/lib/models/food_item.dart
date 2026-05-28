class FoodItem {
  final String itemId;
  final String typeId;
  final String typeName;
  final DateTime expiryDate;
  final int quantity;
  final double weightGram;
  final DateTime registeredAt;
  final int fifoPosition;

  const FoodItem({
    required this.itemId,
    required this.typeId,
    required this.typeName,
    required this.expiryDate,
    required this.quantity,
    required this.weightGram,
    required this.registeredAt,
    required this.fifoPosition,
  });

  factory FoodItem.fromJson(Map<String, dynamic> j) => FoodItem(
        itemId: j['item_id'],
        typeId: j['type_id'],
        typeName: j['type_name'],
        expiryDate: DateTime.parse(j['expiry_date']),
        quantity: j['quantity'],
        weightGram: (j['weight_gram'] as num).toDouble(),
        registeredAt: DateTime.parse(j['registered_at']),
        fifoPosition: j['fifo_position'],
      );

  int get daysUntilExpiry =>
      expiryDate.difference(DateTime.now()).inDays;

  bool get isExpiringSoon => daysUntilExpiry <= 3;
}
