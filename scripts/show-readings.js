require('dotenv').config();
const { createClient } = require('@supabase/supabase-js');

const supabaseAdmin = createClient(
  process.env.SUPABASE_URL,
  process.env.SUPABASE_SERVICE_ROLE_KEY,
  { auth: { autoRefreshToken: false, persistSession: false } }
);

const deviceId = process.argv[2];

if (!deviceId) {
  console.error('Usage: node scripts/show-readings.js <deviceId>');
  process.exit(1);
}

(async () => {
  try {
    console.log('Querying sensor_readings for device:', deviceId);
    const { data: readings, error } = await supabaseAdmin
      .from('sensor_readings')
      .select('*')
      .eq('device_id', deviceId)
      .order('received_at', { ascending: false })
      .limit(20);

    if (error) {
      console.error('Error querying sensor_readings:', error.message || error);
      process.exit(1);
    }

    if (!readings || readings.length === 0) {
      console.log('No sensor_readings found for', deviceId);
      process.exit(0);
    }

    console.log(`Found ${readings.length} reading(s). Showing most recent:`);
    readings.forEach((r, i) => {
      console.log('\n---- Reading', i + 1, '----');
      console.log('received_at:', r.received_at);
      console.log('water_1:', r.water_1, 'water_3:', r.water_3, 'gas_detected:', r.gas_detected);
      console.log('temp_1:', r.temp_1, 'temp_2:', r.temp_2, 'movement:', r.movement);
      console.log('power_status:', r.power_status);
    });
    process.exit(0);
  } catch (err) {
    console.error('Unexpected error:', err.message || err);
    process.exit(1);
  }
})();
