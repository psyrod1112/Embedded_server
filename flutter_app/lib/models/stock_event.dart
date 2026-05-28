class StockOutCandidate {
  final String typeId;
  final String typeName;
  final String itemId;
  final DateTime expiryDate;
  final double unitWeightGram;
  final double weightDiffPct;

  const StockOutCandidate({
    required this.typeId,
    required this.typeName,
    required this.itemId,
    required this.expiryDate,
    required this.unitWeightGram,
    required this.weightDiffPct,
  });

  factory StockOutCandidate.fromJson(Map<String, dynamic> j) =>
      StockOutCandidate(
        typeId: j['type_id'],
        typeName: j['type_name'],
        itemId: j['item_id'],
        expiryDate: DateTime.parse(j['expiry_date']),
        unitWeightGram: (j['unit_weight_gram'] as num).toDouble(),
        weightDiffPct: (j['weight_diff_pct'] as num).toDouble(),
      );
}

class PendingStockOut {
  final String eventId;
  final double deltaGram;
  final List<StockOutCandidate> candidates;
  final DateTime createdAt;

  const PendingStockOut({
    required this.eventId,
    required this.deltaGram,
    required this.candidates,
    required this.createdAt,
  });

  factory PendingStockOut.fromJson(Map<String, dynamic> j) => PendingStockOut(
        eventId: j['event_id'],
        deltaGram: (j['delta_gram'] as num).toDouble(),
        candidates: (j['candidates'] as List)
            .map((c) => StockOutCandidate.fromJson(c))
            .toList(),
        createdAt: DateTime.parse(j['created_at']),
      );
}
