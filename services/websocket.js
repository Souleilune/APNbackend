const { WebSocketServer } = require('ws');
const { createClient } = require('@supabase/supabase-js');

class WebSocketService {
  constructor() {
    this.wss = null;
    this.clients = new Map(); // Map of userId -> Set of WebSocket connections
    this.supabase = null;
  }

  /**
   * Initialize WebSocket server
   * @param {http.Server} server - The HTTP server instance
   */
  initialize(server) {
    // Initialize Supabase client for token verification
    this.supabase = createClient(
      process.env.SUPABASE_URL,
      process.env.SUPABASE_ANON_KEY
    );

    this.wss = new WebSocketServer({ 
      server,
      path: '/ws/telemetry',
    });

    console.log('🔌 WebSocket: Server initialized on path /ws/telemetry');

    this.wss.on('connection', (ws, req) => {
      this._handleConnection(ws, req);
    });

    this.wss.on('error', (error) => {
      console.error('❌ WebSocket Server Error:', error.message);
    });
  }

  /**
   * Handle new WebSocket connections
   * @param {WebSocket} ws - The WebSocket connection
   * @param {http.IncomingMessage} req - The HTTP request
   */
  async _handleConnection(ws, req) {
    console.log('🔗 WebSocket: New connection attempt');

    // Extract token from query string or headers
    const url = new URL(req.url, `http://${req.headers.host}`);
    const token = url.searchParams.get('token') || 
                  req.headers['authorization']?.replace('Bearer ', '');

    if (!token) {
      console.log('❌ WebSocket: No authentication token provided');
      ws.close(4001, 'Authentication required');
      return;
    }

    // Verify token and get user
    try {
      const { data: { user }, error } = await this.supabase.auth.getUser(token);

      if (error || !user) {
        console.log('❌ WebSocket: Invalid or expired token');
        ws.close(4002, 'Invalid token');
        return;
      }

      const userId = user.id;
      ws.userId = userId;
      ws.isAlive = true;

      // Add client to the map
      if (!this.clients.has(userId)) {
        this.clients.set(userId, new Set());
      }
      this.clients.get(userId).add(ws);

      console.log(`✅ WebSocket: User ${userId} connected (${this.clients.get(userId).size} connections)`);

      // Send connection success message
      ws.send(JSON.stringify({
        type: 'connected',
        message: 'Successfully connected to telemetry stream',
        userId,
        timestamp: new Date().toISOString(),
      }));

      // Setup ping-pong for connection health
      ws.on('pong', () => {
        ws.isAlive = true;
      });

      // Handle incoming messages from client
      ws.on('message', (data) => {
        this._handleClientMessage(ws, data);
      });

      // Handle connection close
      ws.on('close', () => {
        this._handleDisconnection(ws, userId);
      });

      // Handle errors
      ws.on('error', (error) => {
        console.error(`❌ WebSocket: Error for user ${userId}:`, error.message);
      });

    } catch (error) {
      console.error('❌ WebSocket: Authentication error:', error.message);
      ws.close(4003, 'Authentication failed');
    }
  }

  /**
   * Handle messages from connected clients
   * @param {WebSocket} ws - The WebSocket connection
   * @param {Buffer} data - The message data
   */
  _handleClientMessage(ws, data) {
    try {
      const message = JSON.parse(data.toString());
      
      switch (message.type) {
        case 'ping':
          ws.send(JSON.stringify({ type: 'pong', timestamp: new Date().toISOString() }));
          break;
        
        case 'subscribe_device':
          // Client wants to subscribe to specific device updates
          if (message.deviceId) {
            ws.subscribedDevices = ws.subscribedDevices || new Set();
            ws.subscribedDevices.add(message.deviceId);
            ws.send(JSON.stringify({ 
              type: 'subscribed', 
              deviceId: message.deviceId,
              timestamp: new Date().toISOString(),
            }));
          }
          break;

        default:
          console.log(`📩 WebSocket: Unknown message type from user ${ws.userId}:`, message.type);
      }
    } catch (error) {
      console.error('❌ WebSocket: Error parsing client message:', error.message);
    }
  }

  /**
   * Handle client disconnection
   * @param {WebSocket} ws - The WebSocket connection
   * @param {string} userId - The user ID
   */
  _handleDisconnection(ws, userId) {
    const userConnections = this.clients.get(userId);
    if (userConnections) {
      userConnections.delete(ws);
      if (userConnections.size === 0) {
        this.clients.delete(userId);
      }
    }
    console.log(`👋 WebSocket: User ${userId} disconnected`);
  }

  /**
   * Send telemetry data to a specific user
   * @param {string} userId - The user ID
   * @param {object} telemetryData - The telemetry data to send
   */
  sendToUser(userId, telemetryData) {
    const userConnections = this.clients.get(userId);
    
    if (!userConnections || userConnections.size === 0) {
      console.log(`📭 WebSocket: No active connections for user ${userId}`);
      return false;
    }

    const message = JSON.stringify({
      type: 'telemetry',
      data: telemetryData,
      timestamp: new Date().toISOString(),
    });

    let sentCount = 0;
    userConnections.forEach((ws) => {
      if (ws.readyState === ws.OPEN) {
        ws.send(message);
        sentCount++;
      }
    });

    console.log(`📤 WebSocket: Sent telemetry to ${sentCount} connection(s) for user ${userId}`);
    return true;
  }

  /**
   * Broadcast telemetry to all connected clients
   * Useful for admin dashboards or system-wide notifications
   * @param {object} data - The data to broadcast
   */
  broadcast(data) {
    const message = JSON.stringify({
      type: 'broadcast',
      data,
      timestamp: new Date().toISOString(),
    });

    let count = 0;
    this.clients.forEach((connections) => {
      connections.forEach((ws) => {
        if (ws.readyState === ws.OPEN) {
          ws.send(message);
          count++;
        }
      });
    });

    console.log(`📢 WebSocket: Broadcast sent to ${count} connections`);
  }

  /**
   * Get connection statistics
   */
  getStats() {
    let totalConnections = 0;
    this.clients.forEach((connections) => {
      totalConnections += connections.size;
    });

    return {
      activeUsers: this.clients.size,
      totalConnections,
    };
  }

  /**
   * Start heartbeat interval to detect stale connections
   */
  startHeartbeat(intervalMs = 30000) {
    setInterval(() => {
      this.wss.clients.forEach((ws) => {
        if (ws.isAlive === false) {
          console.log(`💔 WebSocket: Terminating stale connection for user ${ws.userId}`);
          return ws.terminate();
        }
        ws.isAlive = false;
        ws.ping();
      });
    }, intervalMs);
  }

  /**
   * Gracefully close all connections
   */
  close() {
    this.clients.forEach((connections, userId) => {
      connections.forEach((ws) => {
        ws.close(1001, 'Server shutting down');
      });
    });
    this.clients.clear();
    
    if (this.wss) {
      this.wss.close(() => {
        console.log('👋 WebSocket: Server closed');
      });
    }
  }
}

// Export singleton instance
const websocketService = new WebSocketService();
module.exports = websocketService;
