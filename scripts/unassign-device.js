require('dotenv').config();
const { createClient } = require('@supabase/supabase-js');

const supabaseAdmin = createClient(
  process.env.SUPABASE_URL,
  process.env.SUPABASE_SERVICE_ROLE_KEY,
  { auth: { autoRefreshToken: false, persistSession: false } }
);

const deviceId = process.argv[2];
const force = process.argv.includes('--force');

if (!deviceId) {
  console.error('Usage: node scripts/unassign-device.js <deviceId> [--force]');
  process.exit(1);
}

(async () => {
  try {
    console.log('Querying device:', deviceId);
    const { data: device, error } = await supabaseAdmin
      .from('devices')
      .select('*')
      .eq('device_id', deviceId)
      .maybeSingle();

    if (error) {
      console.error('Error querying devices table:', error.message || error);
      process.exit(1);
    }

    if (!device) {
      console.log('No device row found for', deviceId);
      process.exit(0);
    }

    console.log('Device row:');
    console.log(JSON.stringify(device, null, 2));

    if (!force) {
      console.log('\nRun with --force to clear the user_id / unassign this device.');
      process.exit(0);
    }

    // Perform unassign (set user_id to null)
    const { data: updated, error: updateError } = await supabaseAdmin
      .from('devices')
      .update({ user_id: null, is_active: false })
      .eq('device_id', deviceId)
      .select()
      .single();

    if (updateError) {
      console.error('Error unassigning device:', updateError.message || updateError);
      process.exit(1);
    }

    console.log('Device unassigned:');
    console.log(JSON.stringify(updated, null, 2));
    process.exit(0);
  } catch (err) {
    console.error('Unexpected error:', err.message || err);
    process.exit(1);
  }
})();
