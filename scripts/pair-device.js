/**
 * Utility script to pair a device to a user account
 * Usage: node scripts/pair-device.js <deviceId> <userEmail>
 * Example: node scripts/pair-device.js 841FE869F6AC user@example.com
 */

require('dotenv').config();
const { createClient } = require('@supabase/supabase-js');

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

async function pairDevice(deviceId, userEmail) {
  try {
    console.log(`🔍 Looking up user: ${userEmail}`);
    
    // First, get the user by email from auth
    const { data: authUsers, error: listError } = await supabaseAdmin.auth.admin.listUsers();
    
    if (listError) {
      console.error('❌ Error listing users:', listError.message);
      process.exit(1);
    }
    
    const authUser = authUsers.users.find(u => u.email === userEmail);
    
    if (!authUser) {
      console.error(`❌ User not found: ${userEmail}`);
      console.error('   Available users:');
      authUsers.users.forEach(u => console.error(`   - ${u.email}`));
      process.exit(1);
    }
    
    // Get user from public.users table
    const { data: userData, error: userError } = await supabaseAdmin
      .from('users')
      .select('id, email, full_name')
      .eq('auth_id', authUser.id)
      .maybeSingle();
    
    if (userError) {
      console.error('❌ Error fetching user:', userError.message);
      process.exit(1);
    }
    
    if (!userData) {
      console.error(`❌ User profile not found in database for: ${userEmail}`);
      console.error('   The user exists in auth but not in the users table.');
      process.exit(1);
    }
    
    console.log(`✅ Found user: ${userData.email} (ID: ${userData.id})`);
    console.log(`🔍 Checking if device ${deviceId} is already paired...`);
    
    // Check if device is already paired
    const { data: existingDevice, error: checkError } = await supabaseAdmin
      .from('devices')
      .select('*')
      .eq('device_id', deviceId)
      .maybeSingle();
    
    if (checkError && checkError.code !== 'PGRST116') {
      console.error('❌ Error checking device:', checkError.message);
      process.exit(1);
    }
    
    if (existingDevice) {
      if (existingDevice.user_id === userData.id) {
        console.log(`✅ Device ${deviceId} is already paired to this user.`);
        console.log(`   Device name: ${existingDevice.name}`);
        console.log(`   Paired at: ${existingDevice.paired_at}`);
        process.exit(0);
      } else {
        console.error(`❌ Device ${deviceId} is already paired to another user.`);
        console.error(`   Current user ID: ${existingDevice.user_id}`);
        console.error(`   Requested user ID: ${userData.id}`);
        process.exit(1);
      }
    }
    
    // Pair the device
    console.log(`🔗 Pairing device ${deviceId} to user ${userData.email}...`);
    
    const { data: device, error: createError } = await supabaseAdmin
      .from('devices')
      .insert({
        user_id: userData.id,
        device_id: deviceId,
        name: `Device ${deviceId.slice(-4)}`,
        is_active: true
      })
      .select()
      .single();
    
    if (createError) {
      console.error('❌ Error pairing device:', createError.message);
      process.exit(1);
    }
    
    console.log(`✅ Device paired successfully!`);
    console.log(`   Device ID: ${device.device_id}`);
    console.log(`   Device Name: ${device.name}`);
    console.log(`   User ID: ${device.user_id}`);
    console.log(`   Paired At: ${device.paired_at}`);
    console.log(`   Active: ${device.is_active}`);
    console.log(`\n🎉 Device ${deviceId} is now ready to receive data!`);
    
  } catch (error) {
    console.error('❌ Unexpected error:', error.message);
    console.error(error);
    process.exit(1);
  }
}

// Parse command line arguments
const args = process.argv.slice(2);

if (args.length < 2) {
  console.error('❌ Usage: node scripts/pair-device.js <deviceId> <userEmail>');
  console.error('   Example: node scripts/pair-device.js 841FE869F6AC user@example.com');
  process.exit(1);
}

const [deviceId, userEmail] = args;

if (!deviceId || !userEmail) {
  console.error('❌ Both deviceId and userEmail are required');
  process.exit(1);
}

// Validate environment variables
if (!process.env.SUPABASE_URL || !process.env.SUPABASE_SERVICE_ROLE_KEY) {
  console.error('❌ Missing required environment variables:');
  console.error('   SUPABASE_URL');
  console.error('   SUPABASE_SERVICE_ROLE_KEY');
  console.error('\n   Make sure your .env file is configured correctly.');
  process.exit(1);
}

// Run the pairing function
pairDevice(deviceId, userEmail)
  .then(() => {
    process.exit(0);
  })
  .catch((error) => {
    console.error('❌ Fatal error:', error);
    process.exit(1);
  });

