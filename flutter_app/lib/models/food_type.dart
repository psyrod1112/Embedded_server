class FoodType {
  final String typeId;
  final String typeName;
  final double unitWeightGram;
  final double weightTolerancePct;
  final int totalQuantity;
  final DateTime createdAt;

  const FoodType({
    required this.typeId,
    required this.typeName,
    required this.unitWeightGram,
    required this.weightTolerancePct,
    required this.totalQuantity,
    required this.createdAt,
  });

  factory FoodType.fromJson(Map<String, dynamic> j) => FoodType(
        typeId: j['type_id'],
        typeName: j['type_name'],
        unitWeightGram: (j['unit_weight_gram'] as num).toDouble(),
        weightTolerancePct: (j['weight_tolerance_pct'] as num).toDouble(),
        totalQuantity: j['total_quantity'],
        createdAt: DateTime.parse(j['created_at']),
      );
}
