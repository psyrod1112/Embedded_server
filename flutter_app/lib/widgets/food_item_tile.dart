import 'package:flutter/material.dart';
import 'package:intl/intl.dart';

import '../models/food_item.dart';

class FoodItemTile extends StatelessWidget {
  final FoodItem item;
  final VoidCallback onStockOut;
  final VoidCallback onRestore;

  const FoodItemTile({
    super.key,
    required this.item,
    required this.onStockOut,
    required this.onRestore,
  });

  Color get _expiryColor {
    if (item.daysUntilExpiry <= 1) return Colors.red;
    if (item.daysUntilExpiry <= 3) return Colors.orange;
    return Colors.green;
  }

  @override
  Widget build(BuildContext context) {
    return ListTile(
      leading: CircleAvatar(
        backgroundColor: _expiryColor.withOpacity(0.15),
        child: Text(
          '${item.fifoPosition}',
          style: TextStyle(color: _expiryColor, fontWeight: FontWeight.bold),
        ),
      ),
      title: Row(
        children: [
          Text('유통기한: ${DateFormat('yyyy.MM.dd').format(item.expiryDate)}'),
          const SizedBox(width: 8),
          if (item.isExpiringSoon)
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 6, vertical: 2),
              decoration: BoxDecoration(
                color: _expiryColor,
                borderRadius: BorderRadius.circular(4),
              ),
              child: Text(
                'D-${item.daysUntilExpiry}',
                style: const TextStyle(color: Colors.white, fontSize: 11),
              ),
            ),
        ],
      ),
      subtitle: Text('수량: ${item.quantity}개  |  ${item.weightGram.toStringAsFixed(0)}g/개'),
      trailing: PopupMenuButton<String>(
        onSelected: (v) {
          if (v == 'out') onStockOut();
          if (v == 'restore') onRestore();
        },
        itemBuilder: (_) => [
          const PopupMenuItem(value: 'out', child: Text('출고')),
          const PopupMenuItem(value: 'restore', child: Text('수량 복원')),
        ],
      ),
    );
  }
}
