import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../models/food_item.dart';
import '../models/food_type.dart';
import '../models/stock_event.dart';
import '../services/api_service.dart';

final foodTypesProvider = FutureProvider<List<FoodType>>((ref) {
  return ApiService().getFoodTypes();
});

final foodItemsProvider =
    FutureProvider.family<List<FoodItem>, String>((ref, typeId) {
  return ApiService().getFoodItems(typeId);
});

final pendingStockOutsProvider = FutureProvider<List<PendingStockOut>>((ref) {
  return ApiService().getPendingStockOuts();
});
