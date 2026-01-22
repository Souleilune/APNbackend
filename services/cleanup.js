const cron = require('node-cron');
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

/**
 * Clean up archived alerts older than 7 days
 */
async function cleanupArchivedAlerts() {
  try {
    const sevenDaysAgo = new Date();
    sevenDaysAgo.setDate(sevenDaysAgo.getDate() - 7);
    const cutoffDate = sevenDaysAgo.toISOString();

    console.log(`🧹 Starting cleanup of archived alerts older than ${cutoffDate}`);

    // Delete archived alerts older than 7 days
    const { data, error, count } = await supabaseAdmin
      .from('archived_alerts')
      .delete()
      .lt('archived_at', cutoffDate)
      .select();

    if (error) {
      throw error;
    }

    const deletedCount = data ? data.length : 0;
    console.log(`✅ Cleanup completed: Deleted ${deletedCount} archived alert(s) older than 7 days`);

    return { success: true, deletedCount };
  } catch (error) {
    console.error('❌ Error during cleanup of archived alerts:', error);
    return { success: false, error: error.message };
  }
}

/**
 * Initialize the cleanup cron job
 * Runs daily at 2 AM
 */
function initializeCleanupJob() {
  // Schedule: '0 2 * * *' = Every day at 2:00 AM
  const cronSchedule = process.env.CLEANUP_CRON_SCHEDULE || '0 2 * * *';
  
  console.log(`⏰ Initializing cleanup cron job with schedule: ${cronSchedule}`);
  
  const task = cron.schedule(cronSchedule, async () => {
    console.log(`\n🕐 Cleanup job triggered at ${new Date().toISOString()}`);
    await cleanupArchivedAlerts();
  }, {
    scheduled: true,
    timezone: process.env.TZ || 'UTC'
  });

  console.log('✅ Cleanup cron job initialized successfully');
  
  return task;
}

/**
 * Run cleanup manually (for testing)
 */
async function runCleanupNow() {
  console.log('🔧 Running cleanup manually...');
  return await cleanupArchivedAlerts();
}

module.exports = {
  initializeCleanupJob,
  cleanupArchivedAlerts,
  runCleanupNow
};

