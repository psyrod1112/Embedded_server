import 'package:dio/dio.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../config/app_config.dart';
import '../models/food_item.dart';
import '../models/food_type.dart';
import '../models/stock_event.dart';

class ApiService {
  static final ApiService _instance = ApiService._();
  factory ApiService() => _instance;
  ApiService._();

  final _dio = Dio(BaseOptions(
    baseUrl: AppConfig.baseUrl,
    connectTimeout: const Duration(seconds: 10),
    receiveTimeout: const Duration(seconds: 15),
  ));

  Future<void> _setAuthHeader() async {
    final prefs = await SharedPreferences.getInstance();
    final token = prefs.getString(AppConfig.tokenKey);
    if (token != null) {
      _dio.options.headers['Authorization'] = 'Bearer $token';
    }
  }

  // ── Auth ────────────────────────────────────────────────────────

  Future<String> login(String username, String password) async {
    final resp = await _dio.post('/auth/login', data: {
      'username': username,
      'password': password,
    });
    return resp.data['access_token'] as String;
  }

  Future<void> updateFcmToken(String token) async {
    await _setAuthHeader();
    await _dio.put('/auth/fcm-token', data: {'fcm_token': token});
  }

  // ── Food Types ──────────────────────────────────────────────────

  Future<List<FoodType>> getFoodTypes() async {
    await _setAuthHeader();
    final resp = await _dio.get('/foods/types');
    return (resp.data as List).map((j) => FoodType.fromJson(j)).toList();
  }

  Future<FoodType> createType(String typeName, double unitWeightGram) async {
    await _setAuthHeader();
    final resp = await _dio.post('/foods/types', data: {
      'type_name': typeName,
      'unit_weight_gram': unitWeightGram,
    });
    return FoodType.fromJson(resp.data);
  }

  Future<FoodType> renameType(String typeId, String newName) async {
    await _setAuthHeader();
    final resp = await _dio.patch(
      '/foods/types/$typeId',
      data: {'type_name': newName},
    );
    return FoodType.fromJson(resp.data);
  }

  // ── Food Items ──────────────────────────────────────────────────

  Future<List<FoodItem>> getFoodItems(String typeId) async {
    await _setAuthHeader();
    final resp = await _dio.get('/foods/items', queryParameters: {'type_id': typeId});
    return (resp.data as List).map((j) => FoodItem.fromJson(j)).toList();
  }

  Future<void> manualStockOut(String itemId) async {
    await _setAuthHeader();
    await _dio.post('/foods/items/manual-out', data: {'item_id': itemId});
  }

  Future<void> manualStockIn({
    required String typeId,
    required DateTime expiryDate,
    required int quantity,
    required double weightGram,
  }) async {
    await _setAuthHeader();
    await _dio.post('/foods/items/manual-in', data: {
      'type_id': typeId,
      'expiry_date': expiryDate.toIso8601String().substring(0, 10),
      'quantity': quantity,
      'weight_gram': weightGram,
    });
  }

  Future<void> restoreItem(String itemId) async {
    await _setAuthHeader();
    await _dio.post('/foods/items/restore', data: {'item_id': itemId});
  }

  // ── Pending Stock-Out ────────────────────────────────────────────

  Future<List<PendingStockOut>> getPendingStockOuts() async {
    await _setAuthHeader();
    final resp = await _dio.get('/stock/pending');
    return (resp.data as List).map((j) => PendingStockOut.fromJson(j)).toList();
  }

  Future<void> confirmStockOut(String pendingEventId, String typeId) async {
    await _setAuthHeader();
    await _dio.post('/stock/out/confirm', data: {
      'pending_event_id': pendingEventId,
      'type_id': typeId,
    });
  }
}
