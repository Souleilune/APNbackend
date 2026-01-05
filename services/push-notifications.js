const { Expo } = require('expo-server-sdk');
const { createClient } = require('@supabase/supabase-js');

// Initialize Expo SDK
const expo = new Expo();

// Initialize Supabase Admin client
const supabaseAdmin = createClient(
  process.env.SUPABASE_URL,
  process.env.SUPABASE_SERVICE_ROLE_KEY,
  {
    auth: {
      autoRefreshToken: false,
      persistSession: false
    }
  }
);

// Alert type to notification title mapping (user-friendly)
const ALERT_TITLES = {
  'WATER_DETECTED': '💧 Water Detected',
  'GAS_LEAK_DETECTED': '🚨 Gas Leak Alert',
  'HIGH_TEMPERATURE': '🌡️ High Temperature Warning',
  'GROUND_MOVEMENT_DETECTED': '⚠️ Ground Movement Detected',
  'POWER_ABNORMAL': '⚡ Power Issue Detected',
  'MULTIPLE_HAZARDS': '🚨 Multiple Hazards Detected',
};

// Alert type to user-friendly body message templates
const ALERT_MESSAGES = {
  'WATER_DETECTED': (alert) => {
    const location = alert.sensor?.startsWith('ZONE') 
      ? alert.sensor.replace('ZONE', 'Zone ') 
      : alert.sensor || 'your property';
    return `Water has been detected in ${location}. Please check the area immediately.`;
  },
  'GAS_LEAK_DETECTED': (alert) => {
    return `A gas leak has been detected! Please examine the area immediately and contact emergency services.`;
  },
  'HIGH_TEMPERATURE': (alert) => {
    const temp = alert.value ? `${alert.value.toFixed(1)}°C` : 'an elevated level';
    return `High temperature detected (${temp}). Please check your property for potential fire hazards.`;
  },
  'GROUND_MOVEMENT_DETECTED': (alert) => {
    const intensity = alert.value ? ` (Intensity: ${alert.value.toFixed(2)})` : '';
    return `Ground movement detected${intensity}. This may indicate seismic activity or structural issues.`;
  },
  'POWER_ABNORMAL': (alert) => {
    return `An abnormal power condition has been detected. Please check your electrical system.`;
  },
  'MULTIPLE_HAZARDS': (alert) => {
    return `Multiple hazards have been detected simultaneously. Please check your property immediately and ensure your safety.`;
  },
};

// Alert priority levels
const ALERT_PRIORITY = {
  'GAS_LEAK_DETECTED': 'high',
  'MULTIPLE_HAZARDS': 'high',
  'POWER_ABNORMAL': 'default',
  'GROUND_MOVEMENT_DETECTED': 'default',
  'WATER_DETECTED': 'default',
  'HIGH_TEMPERATURE': 'default',
};

/**
 * Format alert data into a notification message
 */
function formatAlertNotification(alert) {
  const title = ALERT_TITLES[alert.alert_type] || `⚠️ ${alert.alert_type.replace(/_/g, ' ')}`;
  
  // Get user-friendly message
  const messageTemplate = ALERT_MESSAGES[alert.alert_type];
  const body = messageTemplate 
    ? messageTemplate({
        deviceId: alert.device_id,
        sensor: alert.sensor,
        value: alert.value,
      })
    : `Alert detected: ${alert.alert_type}. Please check your device.`;

  return {
    title,
    body,
    priority: ALERT_PRIORITY[alert.alert_type] || 'default',
    sound: 'default',
    data: {
      alertId: alert.id,
      deviceId: alert.device_id,
      alertType: alert.alert_type,
      sensor: alert.sensor,
      value: alert.value,
      receivedAt: alert.received_at,
    },
  };
}

/**
 * Get push tokens for a user
 */
async function getUserPushTokens(userId) {
  try {
    const { data: tokens, error } = await supabaseAdmin
      .from('push_tokens')
      .select('expo_push_token')
      .eq('user_id', userId);

    if (error) {
      console.error('❌ Error fetching push tokens:', error.message);
      return [];
    }

    // Filter out invalid tokens
    const validTokens = tokens
      .map(t => t.expo_push_token)
      .filter(token => Expo.isExpoPushToken(token));

    return validTokens;
  } catch (error) {
    console.error('❌ Error getting user push tokens:', error);
    return [];
  }
}

/**
 * Send push notification to a user
 */
async function sendPushNotificationToUser(userId, alert) {
  try {
    // Get user's push tokens
    const pushTokens = await getUserPushTokens(userId);

    if (pushTokens.length === 0) {
      console.log(`📭 No push tokens found for user ${userId}`);
      return { success: false, reason: 'no_tokens' };
    }

    // Format notification
    const notification = formatAlertNotification(alert);

    // Create messages for all tokens
    const messages = pushTokens.map(token => ({
      to: token,
      ...notification,
    }));

    // Send notifications in chunks (Expo allows up to 100 at a time)
    const chunks = expo.chunkPushNotifications(messages);
    const tickets = [];

    for (const chunk of chunks) {
      try {
        const ticketChunk = await expo.sendPushNotificationsAsync(chunk);
        tickets.push(...ticketChunk);
      } catch (error) {
        console.error('❌ Error sending push notification chunk:', error);
      }
    }

    // Check for errors in tickets
    const errors = tickets.filter(ticket => ticket.status === 'error');
    if (errors.length > 0) {
      console.error('❌ Some push notifications failed:', errors);
      
      // Remove invalid tokens from database
      for (const error of errors) {
        if (error.details && error.details.expoPushToken) {
          const invalidToken = error.details.expoPushToken;
          console.log(`🗑️ Removing invalid token: ${invalidToken}`);
          await supabaseAdmin
            .from('push_tokens')
            .delete()
            .eq('expo_push_token', invalidToken);
        }
      }
    }

    const successCount = tickets.filter(ticket => ticket.status === 'ok').length;
    console.log(`✅ Sent ${successCount}/${tickets.length} push notifications to user ${userId}`);

    return { success: true, sent: successCount, total: tickets.length };
  } catch (error) {
    console.error('❌ Error sending push notification:', error);
    return { success: false, error: error.message };
  }
}

/**
 * Send push notification to multiple users
 */
async function sendPushNotificationToUsers(userIds, alert) {
  const results = await Promise.all(
    userIds.map(userId => sendPushNotificationToUser(userId, alert))
  );

  const successCount = results.filter(r => r.success).length;
  console.log(`✅ Sent push notifications to ${successCount}/${userIds.length} users`);

  return results;
}

module.exports = {
  sendPushNotificationToUser,
  sendPushNotificationToUsers,
  getUserPushTokens,
  formatAlertNotification,
};

