import 'package:flutter/material.dart';

import '../models/food_type.dart';

class FoodTypeCard extends StatelessWidget {
  final FoodType foodType;
  final VoidCallback onTap;

  const FoodTypeCard({super.key, required this.foodType, required this.onTap});

  @override
  Widget build(BuildContext context) {
    return Card(
      margin: const EdgeInsets.symmetric(vertical: 6),
      child: ListTile(
        leading: const Icon(Icons.kitchen, size: 36),
        title: Text(foodType.typeName, style: const TextStyle(fontWeight: FontWeight.bold)),
        subtitle: Text(
          '재고: ${foodType.totalQuantity}개  |  단위무게: ${foodType.unitWeightGram.toStringAsFixed(0)}g',
        ),
        trailing: const Icon(Icons.chevron_right),
        onTap: onTap,
      ),
    );
  }
}
